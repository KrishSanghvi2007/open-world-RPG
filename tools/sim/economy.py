"""Simulates the pigment economy of Act 1 over a full playthrough, from the data files.

    python3 tools/sim/economy.py            prints the tables that docs/BALANCE.md quotes
    python3 tools/sim/economy.py --runs 5000

It plays the act 2,000 times as three kinds of player (hurried, typical, thorough). Each walks the golden path once:
village, the Scumblewood, the Vault, the boss. What differs is how many harvest nodes and caches they find, how many
enemies they kill, how often they rest (resting regrows nodes), and how often they die (a death halves wet pigment).
The question it answers: after one pass, how much of everything on sale can each of them afford?
Scarce means "not all of it". Starving means "not even what the path needs".

Where things are and how many is read from data/*.json; what is hidden in the levels (caches) is listed here,
copied from tools/greybox/build_matriarch_greybox.py.
"""

import json
import os
import random
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
D = {f: json.load(open(os.path.join(ROOT, "data", f + ".json"))) for f in "pigments recipes items attunements enemies quests regions".split()}
PIGMENTS = [p["Name"] for p in D["pigments"]]
YIELD = 2                                             # units per node (AUnderpaintingHarvestNode::Yield)
REGROWS = {p["Name"]: p["Regrowth"] == "OnRest" for p in D["pigments"]}
HALVED = {p["Name"]: p["DeathLoss"] == "Half" for p in D["pigments"]}
ZONES = [r for r in D["regions"] if r["RowType"] == "Zone"]
ENEMY = {e["Name"]: e for e in D["enemies"]}
# Enemies standing in each zone before the Turning (build_matriarch_greybox.py).
ENEMIES = {"VarnishVillage": {}, "Scumblewood": {"Madderback": 6, "Blotter": 6}, "GrisailleVault": {"Pentiment": 6, "Blotter": 4, "ThePalimpsest": 1}}
# Hidden caches (behind one-sided walls), and the ones a quest ending can open.
CACHES = {"Scumblewood": [{"TerreVerte": 2}, {"BoneBlack": 2}, {}, {"Sanguine": 2}, {"LampOil": 1}], "GrisailleVault": [], "VarnishVillage": []}
ON_PATH = {"Madderback": 0.5, "Blotter": 0.7, "Pentiment": 0.6, "ThePalimpsest": 1.0}      # share of each that stands on the road itself

PLAYERS = {
    #            nodes found, caches found, off-path enemies killed, extra rests per zone, deaths in the act
    "hurried":  dict(nodes=0.5, caches=0.0, extra_kills=0.0, rests=0, deaths=4),
    "typical":  dict(nodes=0.75, caches=0.4, extra_kills=0.4, rests=1, deaths=3),
    "thorough": dict(nodes=1.0, caches=1.0, extra_kills=1.0, rests=2, deaths=2),
}


def sinks():
    """Everything pigment can be spent on, once each (consumables: one batch)."""
    rows = []
    for a in D["attunements"]:
        cost = {a["CostPigment"]: a["CostAmount"]}
        if a["TemperPigment"]:
            cost[a["TemperPigment"]] = cost.get(a["TemperPigment"], 0) + a["TemperAmount"]
        rows.append(("attune " + a["Name"], cost, "T%d" % a["Tier"]))
    for r in D["recipes"]:
        cost = {i["Id"]: i["Count"] for i in r["Inputs"] if i["Id"] in PIGMENTS}
        kind = next(i["Type"] for i in D["items"] if i["Name"] == r["OutputItem"])
        rows.append(("make " + r["OutputItem"], cost, kind))
    rows.append(("Bartram's limewash (A Fair Grind)", {"Ceruse": 3}, "Quest"))
    return rows


def play(kind, rng):
    p = PLAYERS[kind]
    wet = {k: 0.0 for k in PIGMENTS}
    dry = {k: 0.0 for k in PIGMENTS}
    lost = {k: 0.0 for k in PIGMENTS}
    deaths_left = p["deaths"]

    def rest():
        for k in PIGMENTS:
            dry[k] += wet[k]
            wet[k] = 0

    def die():
        for k in PIGMENTS:
            if HALVED[k]:
                gone = int(wet[k]) // 2
                wet[k] -= gone
                lost[k] += gone

    first_death_free = True
    for zone in ZONES:
        name = zone["Name"]
        passes = 1 + p["rests"]                     # each rest regrows what regrows
        for visit in range(passes):
            for pigment, count in zip(zone["Pigments"], zone["HarvestNodeCounts"]):
                if visit > 0 and not REGROWS[pigment]:
                    continue
                for _ in range(count):
                    if rng.random() < p["nodes"]:
                        wet[pigment] += YIELD
            if visit == 0:
                for enemy, count in ENEMIES[name].items():
                    for _ in range(count):
                        if rng.random() < ON_PATH[enemy] + (1 - ON_PATH[enemy]) * p["extra_kills"]:
                            for drop in ENEMY[enemy]["Drops"]:
                                if drop["Id"] in PIGMENTS and rng.random() < drop["Chance"]:
                                    wet[drop["Id"]] += drop["Count"]
                for cache in CACHES[name]:
                    if rng.random() < p["caches"]:
                        for k, n in cache.items():
                            if k in PIGMENTS:
                                wet[k] += n
            # Deaths fall in the forest and the Vault, before the rest that would have dried the pigment.
            if name != "VarnishVillage" and deaths_left > 0 and rng.random() < 0.5:
                deaths_left -= 1
                if first_death_free:
                    first_death_free = False     # "the first time, the dark takes nothing"
                else:
                    die()
            rest()
    for quest in D["quests"]:
        if quest["Name"].startswith("MQ_"):
            for reward in quest["RewardPigments"]:
                dry[reward["Id"]] += reward["Count"]
    return dry, lost


def main():
    runs = int(sys.argv[sys.argv.index("--runs") + 1]) if "--runs" in sys.argv else 2000
    rng = random.Random(1)
    rows = sinks()
    total = {k: sum(c.get(k, 0) for _, c, _ in rows) for k in PIGMENTS}
    needed = {"Ochre": 4, "TerreVerte": 0}        # the first quest: 1 for Lamp Oil, 3 for Warm Ground
    print("## What there is to spend pigment on (each once)\n")
    print("| | " + " | ".join(PIGMENTS) + " |")
    print("|---|" + "---|" * len(PIGMENTS))
    for label, kinds in (("Tier 1 attunements (6)", ("T1",)), ("Tier 2 attunements (6)", ("T2",)), ("Weapons, armour, the wick (5)", ("Weapon", "Armour", "Upgrade")),
                         ("One batch of each consumable (5)", ("Consumable",)), ("Bartram's limewash", ("Quest",))):
        print("| %s | %s |" % (label, " | ".join(str(sum(c.get(k, 0) for _, c, t in rows if t in kinds)) for k in PIGMENTS)))
    print("| **Everything** | %s |" % " | ".join("**%d**" % total[k] for k in PIGMENTS))
    print("\n## What one pass through the act brings in (average of %d runs)\n" % runs)
    print("| Player | " + " | ".join(PIGMENTS) + " | Lost to deaths | Share of everything they could buy |")
    print("|---|" + "---|" * (len(PIGMENTS) + 2))
    result = {}
    for kind in PLAYERS:
        income = {k: 0.0 for k in PIGMENTS}
        lost_total = 0.0
        for _ in range(runs):
            dry, lost = play(kind, rng)
            for k in PIGMENTS:
                income[k] += dry[k]
            lost_total += sum(lost.values())
        income = {k: v / runs for k, v in income.items()}
        share = sum(min(income[k], total[k]) for k in PIGMENTS) / sum(total.values())
        result[kind] = (income, share)
        print("| %s | %s | %.1f | %d%% |" % (kind, " | ".join("%.1f" % income[k] for k in PIGMENTS), lost_total / runs, round(share * 100)))
    print("\n## Is anyone starving?\n")
    tier1 = [(label, c) for label, c, t in rows if t == "T1"]
    for kind in PLAYERS:
        income = result[kind][0]
        can = [label.replace("attune ", "") for label, c in tier1 if all(income[k] >= n for k, n in c.items())]
        spare = income["Ochre"] - 4            # the first quest takes 4 Ochre: 1 for Lamp Oil, 3 for Warm Ground
        print("- %s: every first tier within reach but %d (%d of 6); %.1f Ochre left after what the first quest takes; scarcest is %s."
              % (kind, 6 - len(can), len(can), spare, min(PIGMENTS, key=lambda k: income[k] / total[k])))
    return result


if __name__ == "__main__":
    main()
