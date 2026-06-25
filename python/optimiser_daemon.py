#!/usr/bin/env python3
import socket
import traceback
from bayes_opt import BayesianOptimization


HOST = "127.0.0.1"
PORT = 5055

optimizer = None
last_params = None


PARAM_ORDER = ["cf", "ct", "kpz", "kvz", "kiz"]


def make_optimizer(bounds):
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


def handle_command(line):
    global optimizer, last_params

    parts = line.strip().split()
    if not parts:
        return "ERROR empty command"

    cmd = parts[0].upper()

    if cmd == "PING":
        return "OK"

    if cmd == "INIT":
        if len(parts) != 11:
            return "ERROR INIT expects 10 numbers"

        values = [float(x) for x in parts[1:]]

        bounds = {
            "cf":  (values[0], values[1]),
            "ct":  (values[2], values[3]),
            "kpz": (values[4], values[5]),
            "kvz": (values[6], values[7]),
            "kiz": (values[8], values[9]),
        }

        optimizer = make_optimizer(bounds)
        last_params = None

        print("Initialized optimizer with bounds:", bounds, flush=True)
        return "OK"

    if cmd == "ASK":
        if optimizer is None:
            return "ERROR optimizer not initialized"

        last_params = optimizer.suggest()

        return (
            "PARAMS "
            f"{last_params['cf']:.17g} "
            f"{last_params['ct']:.17g} "
            f"{last_params['kpz']:.17g} "
            f"{last_params['kvz']:.17g} "
            f"{last_params['kiz']:.17g}"
        )

    if cmd == "TELL":
        if optimizer is None:
            return "ERROR optimizer not initialized"

        if len(parts) != 7:
            return "ERROR TELL expects 5 params and 1 score"

        cf, ct, kpz, kvz, kiz, score = [float(x) for x in parts[1:]]

        params = {
            "cf": cf,
            "ct": ct,
            "kpz": kpz,
            "kvz": kvz,
            "kiz": kiz,
        }

        # C score is an error/cost: lower is better.
        # bayes_opt maximizes, so register negative score.
        target = -score

        optimizer.register(params=params, target=target)

        best = optimizer.max
        best_params = best["params"]
        best_score = -best["target"]

        print("Registered:", params, "score:", score, flush=True)
        print("Current best:", best_params, "score:", best_score, flush=True)

        return (
            "BEST "
            f"{best_params['cf']:.17g} "
            f"{best_params['ct']:.17g} "
            f"{best_params['kpz']:.17g} "
            f"{best_params['kvz']:.17g} "
            f"{best_params['kiz']:.17g} "
            f"{best_score:.17g}"
        )

    if cmd == "BEST":
        if optimizer is None:
            return "ERROR optimizer not initialized"

        best = optimizer.max
        best_params = best["params"]
        best_score = -best["target"]

        return (
            "BEST "
            f"{best_params['cf']:.17g} "
            f"{best_params['ct']:.17g} "
            f"{best_params['kpz']:.17g} "
            f"{best_params['kvz']:.17g} "
            f"{best_params['kiz']:.17g} "
            f"{best_score:.17g}"
        )

    if cmd == "RESET":
        optimizer = None
        last_params = None
        return "OK"

    if cmd == "SHUTDOWN":
        return "SHUTDOWN"

    return f"ERROR unknown command {cmd}"


def main():
    print(f"optimizer daemon listening on {HOST}:{PORT}", flush=True)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((HOST, PORT))
        server.listen(5)

        while True:
            conn, _ = server.accept()

            with conn:
                raw = conn.recv(4096).decode("utf-8").strip()

                try:
                    reply = handle_command(raw)
                except Exception as e:
                    traceback.print_exc()
                    reply = f"ERROR {e}"

                conn.sendall((reply + "\n").encode("utf-8"))

                if reply == "SHUTDOWN":
                    break


if __name__ == "__main__":
    main()