# View modes

There are several view modes available - most of them apply a preset of panel visibility and/or change options that are available in the ui. There is e.g. a Command & Conquer mode where you can manage the normals that are part of the [vxl](../../Formats.md).

Ace of Spades is another view mode that e.g. doesn't offer a Do-you-want-to-resize popup for the [vxl](../../Formats.md) maps.

Artists that only want to edit voxels without animating them might use the simple layout to get a less complex ui.

## Panel visibility

Changing the view mode writes a preset into the `ve_show*` cvars (for example `ve_showpalette`, `ve_showscript`, `ve_showanimationsettings`, `ve_showbrushes`) and `ui_showconsole`.
Individual panels can then be shown or hidden from the **View** menu or by closing the docked window with its close button.
Those choices persist across sessions (see [configuration](../Configuration.md)).

**Reset layout** rebuilds the default dock layout and re-applies the panel visibility preset for the currently selected view mode.
