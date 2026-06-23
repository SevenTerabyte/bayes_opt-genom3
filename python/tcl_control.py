#!/usr/bin/env python3
import subprocess
import time
import json


PLUGIN = "/home/lichenjiang/openrobots/lib/genom/pocolibs/plugins/bayes_opt"


def tcl_quote_json(obj):
    s = json.dumps(obj)
    # Tcl treats [] specially, so escape them
    return s.replace("[", r"\[").replace("]", r"\]")


class GenomixTcl:
    def __init__(self):
        self.p = subprocess.Popen(
            ["eltclsh"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )

    def cmd(self, line, delay=0.2):
        print(f"\n>>> {line}")
        self.p.stdin.write(line + "\n")
        self.p.stdin.flush()
        time.sleep(delay)

        out = []
        while True:
            self.p.stdout.flush()
            break
        return out

    def setup(self):
        self.cmd("package require genomixd")
        self.cmd(f"genomix rpath /home/lichenjiang/openrobots/lib/genom/pocolibs/plugins")
        self.cmd(f'genomix load {PLUGIN} "" ""')
        self.cmd("set verbose 0")

    def send(self, service, payload=None):
        if payload is None:
            payload = {}
        j = tcl_quote_json(payload)
        self.cmd(f'::genomix::client::bayes_opt send {service} "{j}"')

    def read(self, port):
        self.cmd(f'::genomix::client::bayes_opt read {port} "" ')

    def close(self):
        self.p.terminate()


def main():
    g = GenomixTcl()
    g.setup()

    init_payload = {
        "lower_bounds": [0.0001, 0.000001, 1.0, 1.0, 0.0],
        "upper_bounds": [0.001, 0.0001, 10.0, 10.0, 1.0],
        "max_iterations": 10,
    }

    g.send("Init", init_payload)

    for i in range(3):
        print(f"\n=== Iteration {i} ===")
        g.send("AskNext")
        g.read("params")

        # Later: apply params to controller, wait for experiment, receive measurement
        g.send("UpdateFromMeasure")

        g.send("GetBest")
        g.read("best_result")
        g.read("status")

    # g.send("Reset")
    g.close()


if __name__ == "__main__":
    main()