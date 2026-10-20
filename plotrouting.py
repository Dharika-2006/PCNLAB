import pandas as pd, matplotlib.pyplot as plt

df = pd.read_csv("results.csv", names=["proto", "cond", "pdr", "delay", "ctrl"])
order = ["normal", "congestion", "failure"]
for col, label in [("pdr", "PDR (%)"), ("delay", "End-to-End Delay (ms)"), ("ctrl", "Routing Control Messages")]:
    df.pivot(index="cond", columns="proto", values=col).loc[order].plot(kind="bar", rot=0)
    plt.title(label + " vs Network Condition")
    plt.ylabel(label)
    plt.xlabel("Network Condition")
    plt.savefig(col + ".png")