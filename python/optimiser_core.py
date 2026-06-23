import csv
import json
import math
import os
import random
import statistics
import time
from datetime import datetime

from bayes_opt import BayesianOptimization


PARAM_BOUNDS = {
    "cf": (1e-4, 1e-3),
    "ct": (1e-6, 1e-4),
    "kpz": (1.0, 10.0),
    "kvz": (1.0, 10.0),
    "kiz": (0.0, 1.0),
}

TARGET_Z = 1.0
LOG_PATH = "optimizer_trials.csv"


def compute_z_score(z_values, vz_values, target_z):
    if not z_values:
        return -9999.0

    mean_z_error = sum(abs(z - target_z) for z in z_values) / len(z_values)
    z_variance = statistics.variance(z_values) if len(z_values) > 1 else 0.0
    vz_variance = statistics.variance(vz_values) if len(vz_values) > 1 else 0.0

    return -(
        mean_z_error
        + 0.5 * z_variance
        + 0.2 * vz_variance
    )


def apply_params(cf, ct, kpz, kvz, kiz):
    params = {
        "cf": cf,
        "ct": ct,
        "kpz": kpz,
        "kvz": kvz,
        "kiz": kiz,
    }

    print("Would apply params:", params)
    return params


def get_fake_measurement_window(params, target_z, duration=10.0, sample_period=0.05):
    """
    Temporary fake measurement source.
    Remove this once real PhaseSpace/POM input is connected.
    """
    n = max(2, int(duration / sample_period))

    kpz = params["kpz"]
    kvz = params["kvz"]
    kiz = params["kiz"]

    # Fake behaviour: assume a rough optimum near these values.
    error_bias = (
        0.03 * abs(kpz - 5.0)
        + 0.02 * abs(kvz - 4.0)
        + 0.05 * abs(kiz - 0.3)
    )

    z_values = []
    vz_values = []

    last_z = target_z + error_bias

    for _ in range(n):
        noise = random.gauss(0.0, 0.01)
        z = target_z + error_bias + noise
        vz = (z - last_z) / sample_period

        z_values.append(z)
        vz_values.append(vz)

        last_z = z

    return z_values, vz_values


def get_real_measurement_window(read_pose,
                                duration=10.0,
                                sample_period=0.05):

    z_values = []
    vz_values = []

    start_time = time.time()

    while time.time() - start_time < duration:

        # Not implemented yet. Not sure the syntax
        pose = read_pose()

        z_values.append(pose["z"])
        vz_values.append(pose["vz"])

        time.sleep(sample_period)

    return z_values, vz_values


def objective_from_experiment(cf, ct, kpz, kvz, kiz, target_z=TARGET_Z, fake=True):
    params = apply_params(cf, ct, kpz, kvz, kiz)

    if fake:
        z_values, vz_values = get_fake_measurement_window(
            params=params,
            target_z=target_z,
            duration=10.0,
            sample_period=0.05,
        )
    else:
        z_values, vz_values = get_real_measurement_window(
            duration=10.0,
            sample_period=0.05,
        )

    score = compute_z_score(z_values, vz_values, target_z)

    print(f"Score: {score:.6f}")
    return score


def append_trial_log(path, iteration, params, score):
    file_exists = os.path.exists(path)

    with open(path, "a", newline="") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "time",
                "iteration",
                "cf",
                "ct",
                "kpz",
                "kvz",
                "kiz",
                "score",
            ],
        )

        if not file_exists:
            writer.writeheader()

        writer.writerow({
            "time": datetime.now().isoformat(timespec="seconds"),
            "iteration": iteration,
            "cf": params["cf"],
            "ct": params["ct"],
            "kpz": params["kpz"],
            "kvz": params["kvz"],
            "kiz": params["kiz"],
            "score": score,
        })


def run_optimization(n_iter=10, fake=True):
    optimizer = BayesianOptimization(
        f=None,
        pbounds=PARAM_BOUNDS,
        random_state=1,
        verbose=2,
    )

    for i in range(n_iter):
        print(f"\n=== Iteration {i} ===")

        params = optimizer.suggest()

        score = objective_from_experiment(
            **params,
            target_z=TARGET_Z,
            fake=fake,
        )

        optimizer.register(
            params=params,
            target=score,
        )

        append_trial_log(LOG_PATH, i, params, score)

        print("Current best:", optimizer.max)

    print("\nFinal best:")
    print(json.dumps(optimizer.max, indent=2))

    return optimizer.max


if __name__ == "__main__":
    run_optimization(n_iter=10, fake=True)