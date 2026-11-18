# Devlog 012: style probe, item icons

2026-11-18. A style probe, not a milestone: the first item icons.

![Three item icons](img/012-icon-probe.png)
*Lamp Oil, a Moss Poultice, and a measure of Sanguine, on the satchel's paper.*

## How they are made

By script (`tools/stylekit/paint_icons.py`), the same way the game's surfaces are: a flat underpainting, then short loaded strokes laid along the form. Violet on the shadow side, the item's own colour in the middle, warm cream where the Lamp catches it. There is no outline: the edge is where the strokes stop. Each icon is three simple shapes and about nine seconds of painting for the set.

## What this is for

To answer one question before anyone draws forty of them: can pack items be painted in the Glaze look by rule, and sit on the paper satchel without looking pasted on? I think yes. Your eye is the test.

They are **not in the game**. The satchel still lists items as text. Putting icons on the page is a small job once you have said whether you like them.

## Where it falls short

- The lamp oil's swirl is busier than it should be; glass wants fewer, longer strokes.
- The paper scrap under the pigment is mottled rather than flat.
- Three shapes each is a hard limit on what can be shown. A weapon or a coat would need a drawn silhouette, not ellipses.

This is as far as the probe goes.
