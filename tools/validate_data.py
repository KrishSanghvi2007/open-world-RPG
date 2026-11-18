import json,sys,re
D={}
for f in "pigments recipes items attunements enemies quests regions dialogue lore sounds".split():
    D[f]=json.load(open(f"data/{f}.json"))
    names=[r["Name"] for r in D[f]]
    assert len(names)==len(set(names)),f
    assert all(re.fullmatch(r"[A-Za-z0-9_]+",n) for n in names),f
    keys=[tuple(r.keys()) for r in D[f]]
    assert len(set(keys))==1,(f,"inconsistent keys")
    print(f,len(names))
N={k:{r["Name"] for r in v} for k,v in D.items()}
err=[]
def chk(v,sets,ctx):
    if not any(v in N[s] for s in sets): err.append((ctx,v))
P,I=["pigments"],["items"]
for p in D["pigments"]:
    chk(p["Complement"],P,"comp")
    [chk(z,["regions"],"pz") for z in p["SourceZones"]]
    [chk(i,I,"cu") for i in p["CraftingUses"]]
    used={r["OutputItem"] for r in D["recipes"] if any(x["Id"]==p["Name"] for x in r["Inputs"])}
    if used!=set(p["CraftingUses"]): err.append(("craftinguses",p["Name"],used))
    c=[q for q in D["pigments"] if q["Name"]==p["Complement"]][0]
    if c["Complement"]!=p["Name"]: err.append(("asym",p["Name"]))
    zones={z["Name"] for z in D["regions"] if z["RowType"]=="Zone" and p["Name"] in z["Pigments"]}
    if p["Regrowth"]!="DropOnly" and zones!=set(p["SourceZones"]): err.append(("zones",p["Name"],zones))
for r in D["recipes"]:
    chk(r["OutputItem"],I,"out"); assert r["Name"]=="Recipe_"+r["OutputItem"]
    [chk(x["Id"],P+I,"in") for x in r["Inputs"]]
    if r["UnlockType"]=="Quest": chk(r["UnlockId"],["quests"],"uq")
    elif r["UnlockType"]=="Attunement": chk(r["UnlockId"],["attunements"],"ua")
    else: assert r["UnlockId"]==""
for a in D["attunements"]:
    for k in("Pigment","CostPigment"): chk(a[k],P,k)
    if a["TemperPigment"]:
        chk(a["TemperPigment"],P,"t")
        pg=[q for q in D["pigments"] if q["Name"]==a["Pigment"]][0]
        if pg["Complement"]!=a["TemperPigment"]: err.append(("temper",a["Name"]))
    [chk(x,["attunements"],"pre") for x in a["Prerequisites"]]
hue={p["Hue"]:p for p in D["pigments"]}
for e in D["enemies"]:
    [chk(z,["regions"],"ez") for z in e["Zones"]]
    [chk(x,P,"w") for x in e["Weaknesses"]+e["Resistances"]]
    [chk(x["Id"],P+I,"drop") for x in e["Drops"]]
    hs=[e["Hue"]]+([e["Phase2Hue"]] if e["Phase2Hue"] else [])
    if e["Weaknesses"]!=[hue[h]["Complement"] for h in hs] or e["Resistances"]!=[hue[h]["Name"] for h in hs]: err.append(("hue",e["Name"]))
    for a in e["Attacks"]:
        if a["Windup"]<0.6 or a["Recovery"]<a["Windup"]: err.append(("timing",e["Name"],a["AttackName"]))
    for z in e["Zones"]:
        zr=[r for r in D["regions"] if r["Name"]==z][0]
        if e["Name"] not in zr["Enemies"]: err.append(("zone-enemy",e["Name"],z))
for q in D["quests"]:
    chk(q["Zone"],["regions"],"qz")
    [chk(x,["quests"],"qp") for x in q["Prerequisites"]]
    [chk(x["Id"],I,"ri") for x in q["RewardItems"]]
    [chk(x["Id"],P,"rp") for x in q["RewardPigments"]]
    for c in q["Choices"]:
        if c["CostPigment"]: chk(c["CostPigment"],P,"cc")
        [chk(x["Id"],I,"cri") for x in c["RewardItems"]]
        [chk(x["Id"],P,"crp") for x in c["RewardPigments"]]
    ids=[c["ChoiceId"] for c in q["Choices"]]
    if len(ids)!=len(set(ids)): err.append(("choice-ids",q["Name"]))
    if q["Choices"] and not any(o["Event"]=="Choose" for o in q["Objectives"]): err.append(("choices-without-choose-step",q["Name"]))
EVENTS={"Talk","Have","Craft","Relight","Attune","Kill","Reach","Inspect","Choose"}
NPCS={"HesperVane","BartramSedge","CorinHalfSeen","IlkaThrum"}
for q in D["quests"]:
    if len(q["Objectives"])!=len(q["Steps"]): err.append(("objectives",q["Name"]))
    for o in q["Objectives"]:
        if o["Event"] not in EVENTS or o["Count"]<1: err.append(("objective-event",q["Name"],o["Event"]))
        if o["Event"]=="Talk" and o["Target"] not in NPCS: err.append(("objective-npc",q["Name"],o["Target"]))
        if o["Event"]=="Have": chk(o["Target"],P+I,"have")
        if o["Event"]=="Craft": chk(o["Target"],I,"craft")
        if o["Event"]=="Attune": chk(o["Target"],["attunements"],"attune")
        if o["Event"]=="Kill": chk(o["Target"],["enemies"],"kill")
        if o["Event"]=="Choose": chk(o["Target"],["quests"],"choose")
entries={}
for d in D["dialogue"]:
    if d["Npc"] not in NPCS: err.append(("dialogue-npc",d["Name"]))
    if d["Entry"]>0: entries.setdefault(d["Npc"],0); entries[d["Npc"]]+=1
    if d["Next"] and d["Choices"]: err.append(("dialogue-next-and-choices",d["Name"]))
    for nxt in [d["Next"]]+[c["Next"] for c in d["Choices"]]:
        if nxt: chk(nxt,["dialogue"],"dialogue-next")
    for c in d["Choices"]:
        if c["StartQuest"]: chk(c["StartQuest"],["quests"],"dialogue-quest")
    if len(d["Choices"])>6: err.append(("dialogue-too-many-choices",d["Name"]))
for npc in {d["Npc"] for d in D["dialogue"]}:
    if not entries.get(npc): err.append(("dialogue-no-entry",npc))
for snd in D["sounds"]:
    if not snd["Assets"]: err.append(("sound-no-assets",snd["Name"]))
    if not (0.0 < snd["Volume"] <= 1.5) or snd["PitchMin"] > snd["PitchMax"] or snd["PitchMin"] <= 0: err.append(("sound-numbers",snd["Name"]))
    if not snd["Source"].strip(): err.append(("sound-no-source",snd["Name"]))
    for asset in snd["Assets"]:
        if not (asset.startswith("/Game/ThirdParty/Audio/") or asset.startswith("/Game/Audio/Made/")): err.append(("sound-path",snd["Name"]))
if len({snd["Name"] for snd in D["sounds"]}) != len(D["sounds"]): err.append(("sound-duplicate-name","sounds"))
for l in D["lore"]:
    if l["Kind"] not in {"Marginalia","FieldNote","Ledger"}: err.append(("lore-kind",l["Name"]))
    chk(l["Zone"],["regions"],"lore-zone")
    if not l["Text"].strip() or not l["Title"].strip(): err.append(("lore-empty",l["Name"]))
for r in D["regions"]:
    if r["ParentRegion"]: chk(r["ParentRegion"],["regions"],"par")
    [chk(x,["enemies"],"re") for x in r["Enemies"]+r["EnemiesAfterShift"]]
    [chk(x,P,"rpg") for x in r["Pigments"]]
    assert len(r["Pigments"])==len(r["HarvestNodeCounts"])
# items with craft source have recipes
for i in D["items"]:
    if "Crafted" in i["Source"] and "Recipe_"+i["Name"] not in N["recipes"]: err.append(("norecipe",i["Name"]))
for r in D["recipes"]:
    if r["UnlockType"]=="Quest" and [q for q in D["quests"] if q["Name"]==r["UnlockId"]][0]["SliceScope"]!="Core": err.append(("stretch-unlock",r["Name"]))
for p in D["pigments"]:
    if (p["DeathLoss"]=="None")!=(p["Regrowth"]=="Never"): err.append(("deathloss",p["Name"]))
print("ERRORS:",err or "none")
