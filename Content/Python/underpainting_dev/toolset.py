import contextlib
import io
import os
import runpy

import unreal

import toolset_registry


def _tools_dir() -> str:
    return os.path.realpath(os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "tools"))


@unreal.uclass()
class UnderpaintingDevTools(unreal.ToolsetDefinition):
    """Runs the Underpainting project's own editor build scripts (grey-box and look-test level
    builders, asset importers) that live in the repository's tools/ folder."""

    @toolset_registry.tool_call
    @staticmethod
    def run_script(script: str, args: list[str]) -> str:
        """Runs a Python script from the project's tools/ folder inside the editor.

        Args:
            script: Path of the script relative to tools/, e.g. 'greybox/build_matriarch_greybox.py'.
                    Scripts outside tools/ are refused.
            args: Values the script reads from sys.argv[1:].

        Returns:
            Everything the script printed.
        """
        root = _tools_dir()
        path = os.path.realpath(os.path.join(root, script))
        if os.path.commonpath([root, path]) != root:
            raise ValueError("Script must be inside the project's tools/ folder.")
        if not os.path.isfile(path):
            raise FileNotFoundError("No such script: tools/%s" % script)

        import sys
        output = io.StringIO()
        old_argv = sys.argv
        sys.argv = [path] + list(args)
        try:
            with contextlib.redirect_stdout(output):
                runpy.run_path(path, run_name="__main__")
        finally:
            sys.argv = old_argv
        return output.getvalue()


def register():
    unreal.ToolsetRegistry.register_toolset_class(UnderpaintingDevTools)


def unregister():
    unreal.ToolsetRegistry.unregister_toolset_class(UnderpaintingDevTools)
