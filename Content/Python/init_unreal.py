# Runs when the editor starts. Registers the project's development tools with the MCP server.
# Only the editor has these modules, so a failure here must never stop the project from opening.
try:
    from underpainting_dev import toolset as _dev_toolset
    _dev_toolset.register()
except Exception as error:  # noqa: BLE001
    import unreal
    unreal.log_warning("Underpainting dev tools not registered: %s" % error)
