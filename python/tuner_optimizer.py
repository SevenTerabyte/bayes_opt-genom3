#!/usr/bin/env python3
"""
tuner_optimizer.py

Standalone per-call Bayesian optimizer helper for drone autotuning.

Expected usage from C / GenoM / auto-tuner component:

    python3 tuner_optimizer.py \
        --input current_trial.json \
        --output next_params.json \
        --state optimizer_state.json \
        --bounds bounds.json

The script:
  1. Reads one flight trial JSON.
  2. Extracts 20s flight data from JSON.
  3. Computes selected flight metrics.
  4. Converts metrics into one scalar score/cost.
  5. Stores trial history in a persistent state JSON.
  6. Rebuilds Bayesian optimizer from previous trials.
  7. Suggests the next 15-parameter set.
  8. Writes an output JSON containing:
       - complete flag
       - next_params dictionary
       - current score/metrics
       - best score/params so far

Important:
  - Lower score is better.
  - bayesian-optimization maximizes, so internally this script registers target = -score.
  - read_flight_data() is the only part expected to change once the real input JSON format is confirmed.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import random
from dataclasses import dataclass
from typing import Any, Dict, List, Optional, Tuple

import numpy as np
from bayes_opt import BayesianOptimization


# =============================================================================
# Configuration
# =============================================================================

VERY_BAD_SCORE = 100000.0

# Replace these with real parameter names/bounds once confirmed.
# Format:
#   "parameter_name": (lower_bound, upper_bound)
DEFAULT_PARAM_BOUNDS = {
    "Kpx": (2.5, 7.5),
    "Kpy": (2.5, 7.5),
    "Kpz": (14.0, 42.0),

    "Kqx": (2.0, 6.0),
    "Kqy": (2.0, 6.0),
    "Kqz": (0.05, 0.2),

    "Kvx": (3.0, 9.0),
    "Kvy": (3.0, 9.0),
    "Kvz": (4.5, 13.5),

    "Kwx": (0.5, 1.5),
    "Kwy": (0.5, 1.5),
    "Kwz": (0.05, 0.2),

    "Kix": (0.0, 0.5),
    "Kiy": (0.0, 0.5),
    "Kiz": (0.0, 2.0),
}

DEFAULT_TARGET = {
    "x": 0.0,
    "y": 0.0,
    "z": 1.0,
}

# First-version score:
#
# score =
#     5 * steady_state_error
#   + 3 * RMSE
#   + 2 * overshoot
#   + 2 * settling_time
#   + 1 * velocity_smoothness
#
# if crash/timeout/constraint violation:
#     score = VERY_BAD_SCORE
WEIGHTS = {
    "steady_state_error": 5.0,
    "rmse": 3.0,
    "overshoot": 2.0,
    "settling_time": 2.0,
    "velocity_smoothness": 1.0,
}

SETTLING_TOLERANCE_M = 0.05
RISE_FRACTION = 0.90
MIN_RANDOM_TRIALS = 5


# =============================================================================
# Data containers
# =============================================================================

@dataclass
class FlightData:
    t: np.ndarray
    x: np.ndarray
    y: np.ndarray
    z: np.ndarray
    vx: Optional[np.ndarray] = None
    vy: Optional[np.ndarray] = None
    vz: Optional[np.ndarray] = None


# =============================================================================
# JSON helpers
# =============================================================================

def load_json(path: str, default: Any = None) -> Any:
    if not path or not os.path.exists(path):
        return default

    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def load_input_json_or_jsonl(path):
    with open(path, "r", encoding="utf-8") as f:
        text = f.read().strip()

    if not text:
        return {}

    try:
        return json.loads(text)
    except json.JSONDecodeError:
        samples = []
        for line in text.splitlines():
            line = line.strip()
            if not line:
                continue
            samples.append(json.loads(line))
        return {
            "flight_complete": True,
            "sample_rate_hz": 960.0,
            "flight_data": {
                "samples": samples,
            },
        }


def save_json(path: str, obj: Any) -> None:
    directory = os.path.dirname(os.path.abspath(path))
    if directory:
        os.makedirs(directory, exist_ok=True)

    tmp_path = path + ".tmp"

    with open(tmp_path, "w", encoding="utf-8") as f:
        json.dump(obj, f, indent=2)

    os.replace(tmp_path, path)


def load_bounds(path: Optional[str]) -> Dict[str, Tuple[float, float]]:
    if not path:
        return DEFAULT_PARAM_BOUNDS

    raw = load_json(path, default=None)
    if raw is None:
        return DEFAULT_PARAM_BOUNDS

    bounds: Dict[str, Tuple[float, float]] = {}

    for name, pair in raw.items():
        if not isinstance(pair, list) and not isinstance(pair, tuple):
            raise ValueError(f"Invalid bounds for {name}: {pair}")
        if len(pair) != 2:
            raise ValueError(f"Invalid bounds for {name}: {pair}")

        low = float(pair[0])
        high = float(pair[1])

        if not low < high:
            raise ValueError(f"Lower bound must be < upper bound for {name}: {pair}")

        bounds[name] = (low, high)

    return bounds


# =============================================================================
# Flight data adapter
# =============================================================================

def timestamp_to_seconds(ts):
    if not ts:
        return None
    return float(ts.get("sec", 0)) + 1e-9 * float(ts.get("nsec", 0))


def read_flight_data(input_json):
    container = input_json.get("flight_data", input_json)
    samples = container.get("samples")

    if samples is None and isinstance(container, list):
        samples = container

    if not samples:
        return None

    sample_rate_hz = float(input_json.get("sample_rate_hz", 960.0))

    t_list = []
    x_list = []
    y_list = []
    z_list = []

    for i, sample in enumerate(samples):
        try:
            sample_index = sample.get("sample_index", i)
            t = float(sample_index) / sample_rate_hz

            pos = sample.get("position", {})

            t_list.append(t)
            x_list.append(float(pos["x"]))
            y_list.append(float(pos["y"]))
            z_list.append(float(pos["z"]))

        except (KeyError, TypeError, ValueError):
            continue

    if len(t_list) < 2:
        return None

    t = np.asarray(t_list, dtype=float)
    x = np.asarray(x_list, dtype=float)
    y = np.asarray(y_list, dtype=float)
    z = np.asarray(z_list, dtype=float)

    order = np.argsort(t)

    t = t[order]
    x = x[order]
    y = y[order]
    z = z[order]

    vx, vy, vz = estimate_velocity(t, x, y, z)

    return FlightData(t=t, x=x, y=y, z=z, vx=vx, vy=vy, vz=vz)


def estimate_velocity(
    t: np.ndarray,
    x: np.ndarray,
    y: np.ndarray,
    z: np.ndarray,
) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    dt = np.gradient(t)

    # Avoid division by zero or broken timestamps.
    dt = np.where(np.abs(dt) < 1e-9, 1e-9, dt)

    vx = np.gradient(x) / dt
    vy = np.gradient(y) / dt
    vz = np.gradient(z) / dt

    return vx, vy, vz


# =============================================================================
# Metric functions
# =============================================================================

def compute_position_error(data: FlightData, target: Dict[str, float]) -> np.ndarray:
    dx = data.x - float(target["x"])
    dy = data.y - float(target["y"])
    dz = data.z - float(target["z"])

    return np.sqrt(dx * dx + dy * dy + dz * dz)


def compute_steady_state_error(
    data: Optional[FlightData],
    target: Dict[str, float],
) -> float:
    """
    Late average position error from target.
    Uses the final 20% of the flight.
    """

    if data is None:
        return 0.0

    err = compute_position_error(data, target)

    start = int(0.8 * len(err))
    late_err = err[start:]

    if len(late_err) == 0:
        return float(np.mean(err))

    return float(np.mean(late_err))


def compute_rmse(
    data: Optional[FlightData],
    target: Dict[str, float],
) -> float:
    """
    Overall RMS position tracking error.
    """

    if data is None:
        return 0.0

    err = compute_position_error(data, target)
    return float(np.sqrt(np.mean(err * err)))


def compute_overshoot(
    data: Optional[FlightData],
    target: Dict[str, float],
) -> float:
    """
    First version: z-axis overshoot above target z.

    For a hover or vertical step, this is simple and useful.

    Later, this can be replaced with a direction-aware overshoot metric for
    arbitrary 3D target changes.
    """

    if data is None:
        return 0.0

    target_z = float(target["z"])
    max_z = float(np.max(data.z))

    return max(0.0, max_z - target_z)


def compute_settling_time(
    data: Optional[FlightData],
    target: Dict[str, float],
    tolerance: float = SETTLING_TOLERANCE_M,
) -> float:
    """
    Time after which position error remains inside tolerance.

    If it never settles, returns full flight duration.
    """

    if data is None:
        return 0.0

    err = compute_position_error(data, target)
    t = data.t

    if len(t) < 2:
        return 0.0

    duration = float(t[-1] - t[0])
    inside = err <= tolerance

    for i in range(len(inside)):
        if np.all(inside[i:]):
            return float(t[i] - t[0])

    return duration


def compute_velocity_smoothness(data: Optional[FlightData]) -> float:
    """
    Penalize sudden velocity changes.

    First version:
      mean norm of acceleration estimated from velocity derivative.

    This is not exactly jerk yet. It is a useful first smoothness penalty.
    Later it can become:
      - mean acceleration norm
      - RMS acceleration norm
      - jerk norm
      - filtered acceleration / jerk
      - control-command smoothness
    """

    if data is None or data.vx is None or data.vy is None or data.vz is None:
        return 0.0

    t = data.t
    dt = np.gradient(t)
    dt = np.where(np.abs(dt) < 1e-9, 1e-9, dt)

    ax = np.gradient(data.vx) / dt
    ay = np.gradient(data.vy) / dt
    az = np.gradient(data.vz) / dt

    acc_norm = np.sqrt(ax * ax + ay * ay + az * az)

    return float(np.mean(acc_norm))


def compute_rise_time(
    data: Optional[FlightData],
    target: Dict[str, float],
    fraction: float = RISE_FRACTION,
) -> Optional[float]:
    """
    Optional metric.

    For z step response:
      time to first reach fraction * target_z.

    Not used in first-version score.
    """

    if data is None:
        return None

    target_z = float(target["z"])
    threshold = fraction * target_z

    for ti, zi in zip(data.t, data.z):
        if zi >= threshold:
            return float(ti - data.t[0])

    return None


def compute_peak_error(
    data: Optional[FlightData],
    target: Dict[str, float],
) -> Optional[float]:
    """
    Optional metric.
    """

    if data is None:
        return None

    err = compute_position_error(data, target)
    return float(np.max(err))


def compute_extra_metrics_placeholder(
    data: Optional[FlightData],
    input_json: Dict[str, Any],
) -> Dict[str, Optional[float]]:
    """
    Space for future metrics:
      - jerk
      - control effort
      - control smoothness
      - attitude error
      - motor saturation
      - angular velocity
      - battery voltage effect
    """

    return {
        "jerk": None,
        "control_effort": None,
        "control_smoothness": None,
        "attitude_error": None,
    }


def compute_metrics(input_json: Dict[str, Any]) -> Dict[str, Any]:
    target = input_json.get("target", DEFAULT_TARGET)
    data = read_flight_data(input_json)

    crash = bool(input_json.get("crash", False))
    timeout = bool(input_json.get("timeout", False))
    constraint_violation = bool(input_json.get("constraint_violation", False))

    metrics: Dict[str, Any] = {
        "crash": crash,
        "timeout": timeout,
        "constraint_violation": constraint_violation,
        "steady_state_error": compute_steady_state_error(data, target),
        "rmse": compute_rmse(data, target),
        "overshoot": compute_overshoot(data, target),
        "settling_time": compute_settling_time(data, target),
        "velocity_smoothness": compute_velocity_smoothness(data),
        "rise_time": compute_rise_time(data, target),
        "peak_error": compute_peak_error(data, target),
    }

    metrics.update(compute_extra_metrics_placeholder(data, input_json))

    metrics["data_available"] = data is not None
    metrics["sample_count"] = int(len(data.t)) if data is not None else 0

    return metrics


# =============================================================================
# Score function
# =============================================================================

def compute_score(metrics: Dict[str, Any]) -> float:
    """
    Lower score is better.

    Hard failure:
      crash / timeout / constraint violation -> VERY_BAD_SCORE

    Otherwise:
      weighted score from selected metrics.
    """

    if metrics.get("crash") or metrics.get("timeout"):
        return VERY_BAD_SCORE

    if metrics.get("constraint_violation"):
        return VERY_BAD_SCORE

    score = 0.0

    score += WEIGHTS["steady_state_error"] * float(metrics["steady_state_error"])
    score += WEIGHTS["rmse"] * float(metrics["rmse"])
    score += WEIGHTS["overshoot"] * float(metrics["overshoot"])
    score += WEIGHTS["settling_time"] * float(metrics["settling_time"])
    score += WEIGHTS["velocity_smoothness"] * float(metrics["velocity_smoothness"])

    return float(score)


# =============================================================================
# Optimizer
# =============================================================================

def make_optimizer(bounds: Dict[str, Tuple[float, float]]) -> BayesianOptimization:
    try:
        return BayesianOptimization(
            f=None,
            pbounds=bounds,
            random_state=1,
            verbose=0,
            allow_duplicate_points=True,
        )
    except TypeError:
        # Older bayesian-optimization versions may not support allow_duplicate_points.
        return BayesianOptimization(
            f=None,
            pbounds=bounds,
            random_state=1,
            verbose=0,
        )


def random_params(bounds: Dict[str, Tuple[float, float]]) -> Dict[str, float]:
    return {
        name: random.uniform(low, high)
        for name, (low, high) in bounds.items()
    }


def rebuild_optimizer_from_trials(
    bounds: Dict[str, Tuple[float, float]],
    trials: List[Dict[str, Any]],
) -> BayesianOptimization:
    optimizer = make_optimizer(bounds)

    for trial in trials:
        params = trial.get("params")
        score = trial.get("score")

        if not params or score is None:
            continue

        safe_params: Dict[str, float] = {}
        for name in bounds:
            if name not in params:
                continue
            safe_params[name] = float(params[name])

        if len(safe_params) != len(bounds):
            continue

        # bayesian-optimization maximizes. We minimize score.
        target = -float(score)

        try:
            optimizer.register(params=safe_params, target=target)
        except Exception:
            # Ignore malformed or duplicate historical points.
            pass

    return optimizer


def suggest_next_params(
    bounds: Dict[str, Tuple[float, float]],
    trials: List[Dict[str, Any]],
    min_random_trials: int = MIN_RANDOM_TRIALS,
) -> Dict[str, float]:
    """
    Suggest next parameter set.

    Uses random exploration for the first few trials.
    Then uses BayesianOptimization.suggest().
    """

    completed_trials = [
        t for t in trials
        if t.get("params") is not None and t.get("score") is not None
    ]

    if len(completed_trials) < min_random_trials:
        return random_params(bounds)

    optimizer = rebuild_optimizer_from_trials(bounds, completed_trials)

    try:
        suggestion = optimizer.suggest()
        return {name: float(suggestion[name]) for name in bounds}
    except Exception:
        # Safe fallback.
        return random_params(bounds)


# =============================================================================
# Main per-call logic
# =============================================================================

def process_trial(
    input_json: Dict[str, Any],
    state_json: Dict[str, Any],
    bounds: Dict[str, Tuple[float, float]],
) -> Dict[str, Any]:
    trials = state_json.setdefault("trials", [])

    current_params = input_json.get("current_params")
    has_completed_flight = bool(input_json.get("flight_complete", True))

    metrics = compute_metrics(input_json)
    score = compute_score(metrics)

    trial_recorded = False

    if current_params and has_completed_flight:
        trial_params: Dict[str, float] = {}

        for name in bounds:
            if name in current_params:
                trial_params[name] = float(current_params[name])

        if len(trial_params) == len(bounds):
            trial = {
                "iteration": len(trials),
                "params": trial_params,
                "metrics": metrics,
                "score": score,
                "complete": True,
            }

            trials.append(trial)
            trial_recorded = True

    next_params = suggest_next_params(bounds, trials)

    completed_trials = [
        t for t in trials
        if t.get("complete") and t.get("score") is not None
    ]

    best_trial = None
    if completed_trials:
        best_trial = min(completed_trials, key=lambda t: float(t["score"]))

    output = {
        "complete": True,
        "trial_recorded": trial_recorded,
        "next_params": next_params,
        "current_score": score if current_params else None,
        "current_metrics": metrics if current_params else None,
        "best": {
            "score": best_trial["score"],
            "params": best_trial["params"],
            "metrics": best_trial["metrics"],
            "iteration": best_trial["iteration"],
        } if best_trial else None,
        "num_trials": len(trials),
        "message": "ok",
    }

    return output


def main() -> int:
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--input",
        required=True,
        help="Input JSON from C containing current params and flight data.",
    )

    parser.add_argument(
        "--output",
        required=True,
        help="Output JSON path for suggested next params.",
    )

    parser.add_argument(
        "--state",
        required=True,
        help="Persistent optimizer state JSON path.",
    )

    parser.add_argument(
        "--bounds",
        default=None,
        help="Optional bounds JSON path. If omitted, DEFAULT_PARAM_BOUNDS is used.",
    )

    args = parser.parse_args()

    try:
        input_json = load_input_json_or_jsonl(args.input)
        state_json = load_json(args.state, default={"trials": []})
        bounds = load_bounds(args.bounds)

        output_json = process_trial(input_json, state_json, bounds)

        save_json(args.state, state_json)
        save_json(args.output, output_json)

        return 0

    except Exception as e:
        error_output = {
            "complete": False,
            "next_params": None,
            "message": f"error: {e}",
        }

        save_json(args.output, error_output)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())