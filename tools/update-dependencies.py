import os
import subprocess
import shutil
import json
import glob

UPDATEDIR = "./update_dir"
HASHFILE = ".dependencies.json"

# Load or initialize the hash file
def load_hashes():
    if os.path.exists(HASHFILE):
        with open(HASHFILE, "r") as f:
            return json.load(f)
    return {}

def save_hashes(hashes):
    with open(HASHFILE, "w") as f:
        json.dump(hashes, f, indent=4)

def get_git_revision(directory):
    try:
        return subprocess.check_output(
            ["git", "-C", directory, "rev-parse", "HEAD"],
            text=True
        ).strip()
    except subprocess.CalledProcessError:
        return None

def update_git(name, repo_url, branch=None):
    sync_dir = os.path.join(UPDATEDIR, f"{name}")
    if not os.path.exists(sync_dir):
        clone_cmd = ["git", "clone", "--recursive", "--depth=1", repo_url, sync_dir]
        if branch:
            clone_cmd += ["-b", branch]
        subprocess.run(clone_cmd, check=True)
    else:
        pull_cmd = ["git", "-C", sync_dir, "pull", "--depth=1", "--rebase"]
        subprocess.run(pull_cmd, check=True)
    return get_git_revision(sync_dir)

def convert_to_unix_line_endings(src, dest):
    """
    Reads the source file, converts its line endings to Unix (\n),
    and writes it to the destination.
    """
    try:
        with open(src, 'r', newline=None) as infile, open(dest, 'w', newline='\n') as outfile:
            for line in infile:
                outfile.write(line.rstrip() + '\n')
    except Exception as e:
        print(f"Failed to copy and convert line endings: {src} -> {dest}")
        print(e)
        return
    print(f"Copied and converted line endings: {src} -> {dest}")

def copy_file(src_pattern, destination):
    """
    Copies files matching the src_pattern to the destination directory.
    Handles wildcards in src_pattern.
    """
    matched_files = glob.glob(src_pattern)
    if not matched_files:
        print(f"No files matched the pattern: {src_pattern}")
        return

    for src in matched_files:
        if os.path.isdir(destination):
            dest = os.path.join(destination, os.path.basename(src))
        else:
            dest = destination
        convert_to_unix_line_endings(src, dest)

def copy_directory(src_pattern, dest):
    """
    Copies directories matching the src_pattern to the destination path.
    Handles wildcards in src_pattern.
    """
    matched_dirs = glob.glob(src_pattern)
    if not matched_dirs:
        print(f"No directories matched the pattern: {src_pattern}")
        return

    for src in matched_dirs:
        target_dest = dest if len(matched_dirs) == 1 else os.path.join(dest, os.path.basename(src))
        if os.path.exists(target_dest):
            shutil.rmtree(target_dest)
        shutil.copytree(src, target_dest)
        print(f"Copied directory: {src} -> {target_dest}")

def copy_if_updated(src, dest):
    if os.path.isdir(src):
        copy_directory(src, dest)
    else:
        copy_file(src, dest)

def update_target(name, repo_url, src_dest_pairs, branch=None):
    hashes = load_hashes()
    current_hash = update_git(name, repo_url, branch)
    if hashes.get(name) == current_hash:
        print(f"{name} is up to date with hash {current_hash} vs {hashes.get(name)}")
        return
    hashes[name] = current_hash
    for src, dest in src_dest_pairs:
        copy_if_updated(f"{UPDATEDIR}/{name}/{src}", dest)
    save_hashes(hashes)

# Define individual update functions
def update_emscripten_browser_file():
    update_target(
        "emscripten-browser-file",
        "https://github.com/Armchair-Software/emscripten-browser-file.git",
        [(f"emscripten_browser_file.h",
          "src/modules/io/system/emscripten_browser_file.h")]
    )

def update_stb():
    update_target(
        "SOIL2",
        "https://github.com/SpartanJ/SOIL2.git",
        [(f"src/SOIL2", "contrib/libs/stb_image")]
    )
    update_target(
        "stb",
        "https://github.com/nothings/stb.git",
        [
            (f"stb_truetype.h", "src/modules/voxelfont/external/stb_truetype.h"),
            (f"stb_rect_pack.h", "src/modules/scenegraph/external/stb_rect_pack.h"),
            (f"stb_image_resize2.h", "contrib/libs/stb_image")
        ]
    )

def update_googletest():
    update_target(
        "googletest",
        "https://github.com/google/googletest.git",
        [
            (f"googletest/src", "contrib/libs/gtest/src"),
            (f"googletest/include", "contrib/libs/gtest/include"),
            (f"googlemock/src", "contrib/libs/gtest/src"),
            (f"googlemock/include", "contrib/libs/gtest/include")
        ]
    )

def update_benchmark():
    update_target(
        "benchmark",
        "https://github.com/google/benchmark.git",
        [
            (f"src", "contrib/libs/benchmark/src"),
            (f"include", "contrib/libs/benchmark/include")
        ]
    )

# Add similar functions for the remaining targets
# Example:
def update_imguizmo():
    update_target(
        "imguizmo",
        "https://github.com/CedricGuillemet/ImGuizmo.git",
        [(f"ImGuizmo.*", "src/modules/ui/dearimgui")]
    )

def update_implot():
    update_target(
        "implot",
        "https://github.com/epezent/implot.git",
        [
            (f"implot.*", "src/modules/ui/dearimgui"),
            (f"implot_internal.h", "src/modules/ui/dearimgui"),
            (f"implot_items.cpp", "src/modules/ui/dearimgui"),
            (f"implot_demo.cpp", "src/tests/testimgui")
        ]
    )

def update_backward():
    update_target(
        "backward-cpp",
        "https://github.com/bombela/backward-cpp.git",
        [
            (f"backward.cpp", "contrib/libs/backward"),
            (f"backward.hpp", "contrib/libs/backward/backward.h")
        ]
    )

def update_im_neo_sequencer():
    update_target(
        "im-neo-sequencer",
        "https://gitlab.com/GroGy/im-neo-sequencer.git",
        [
            (f"imgui*.cpp", "src/modules/ui/dearimgui"),
            (f"imgui*.h", "src/modules/ui/dearimgui"),
            (f"LICENSE", "src/modules/ui/dearimgui/LICENSE-sequencer")
        ]
    )

def update_dearimgui():
    update_target(
        "imgui",
        "https://github.com/ocornut/imgui.git",
        [
            (f"im*.h", "src/modules/ui/dearimgui"),
            (f"im*.cpp", "src/modules/ui/dearimgui"),
            (f"misc/cpp/*", "src/modules/ui/dearimgui"),
            (f"backends/imgui_impl_sdl2.*", "src/modules/ui/dearimgui/backends"),
            (f"backends/imgui_impl_opengl3*", "src/modules/ui/dearimgui/backends"),
            (f"examples/example_sdl2_opengl3/main.cpp", "src/modules/ui/dearimgui/backends/example_sdl2_opengl3.cpp"),
            (f"misc/fonts/binary_to_compressed_c.cpp", "tools/binary_to_compressed_c"),
            (f"misc/freetype/*", "src/modules/ui/dearimgui/misc/freetype")
        ],
        branch="docking"
    )

def update_glm():
    update_target(
        "glm",
        "https://github.com/g-truc/glm.git",
        [(f"glm", "contrib/libs/glm/glm")]
    )

def update_sdl2():
    update_target(
        "sdl2",
        "https://github.com/libsdl-org/SDL.git",
        [
            (f"CMakeLists.txt", "contrib/libs/sdl2"),
            (f"*.cmake.in", "contrib/libs/sdl2"),
            (f"src/*", "contrib/libs/sdl2/src"),
            (f"include/*", "contrib/libs/sdl2/include"),
            (f"cmake/*", "contrib/libs/sdl2/cmake"),
            (f"wayland-protocols/*", "contrib/libs/sdl2/wayland-protocols")
        ],
        branch="SDL2"
    )

def update_tinygltf():
    update_target(
        "tinygltf",
        "https://github.com/syoyo/tinygltf.git",
        [
            (f"tiny_gltf.h", "src/modules/voxelformat/external"),
            (f"json.hpp", "src/modules/json/private")
        ]
    )

def update_tinyobjloader():
    update_target(
        "tinyobjloader",
        "https://github.com/tinyobjloader/tinyobjloader.git",
        [(f"tiny_obj_loader.h", "src/modules/voxelformat/external")]
    )

def update_simplecpp():
    update_target(
        "simplecpp",
        "https://github.com/danmar/simplecpp.git",
        [(f"simplecpp.*", "contrib/libs/simplecpp")]
    )

def update_simplexnoise():
    update_target(
        "simplexnoise",
        "https://github.com/simongeilfus/SimplexNoise.git",
        [(f"include/Simplex.h", "src/modules/noise")]
    )

def update_flextgl():
    update_target(
        "flextgl",
        "https://github.com/mosra/flextgl.git",
        [
            (f"*.py", "tools/flextGL"),
            (f"README.md", "tools/flextGL"),
            (f"COPYING", "tools/flextGL"),
            (f"templates/sdl", "tools/flextGL/templates/sdl"),
            (f"templates/vulkan-dynamic", "tools/flextGL/templates/vulkan-dynamic")
        ]
    )

def update_libvxl():
    update_target(
        "libvxl",
        "https://github.com/xtreme8000/libvxl.git",
        [
            (f"libvxl.c", "src/modules/voxelformat/external"),
            (f"libvxl.h", "src/modules/voxelformat/external")
        ]
    )

def update_meshoptimizer():
    update_target(
        "meshoptimizer",
        "https://github.com/zeux/meshoptimizer.git",
        [(f"src/*", "contrib/libs/meshoptimizer")]
    )

def update_yocto():
    update_target(
        "yocto-gl",
        "https://github.com/xelatihy/yocto-gl.git",
        [
            (f"libs/yocto/yocto_bvh.cpp", "contrib/libs/yocto/yocto_bvh.cpp"),
            (f"libs/yocto/yocto_bvh.h", "contrib/libs/yocto/yocto_bvh.h"),
            (f"libs/yocto/yocto_color.h", "contrib/libs/yocto/yocto_color.h"),
            (f"libs/yocto/yocto_geometry.h", "contrib/libs/yocto/yocto_geometry.h"),
            (f"libs/yocto/yocto_image.cpp", "contrib/libs/yocto/yocto_image.cpp"),
            (f"libs/yocto/yocto_image.h", "contrib/libs/yocto/yocto_image.h"),
            (f"libs/yocto/yocto_math.h", "contrib/libs/yocto/yocto_math.h"),
            (f"libs/yocto/yocto_noise.h", "contrib/libs/yocto/yocto_noise.h"),
            (f"libs/yocto/yocto_parallel.h", "contrib/libs/yocto/yocto_parallel.h"),
            (f"libs/yocto/yocto_sampling.h", "contrib/libs/yocto/yocto_sampling.h"),
            (f"libs/yocto/yocto_scene.cpp", "contrib/libs/yocto/yocto_scene.cpp"),
            (f"libs/yocto/yocto_scene.h", "contrib/libs/yocto/yocto_scene.h"),
            (f"libs/yocto/yocto_shading.h", "contrib/libs/yocto/yocto_shading.h"),
            (f"libs/yocto/yocto_shape.cpp", "contrib/libs/yocto/yocto_shape.cpp"),
            (f"libs/yocto/yocto_shape.h", "contrib/libs/yocto/yocto_shape.h"),
            (f"libs/yocto/yocto_trace.cpp", "contrib/libs/yocto/yocto_trace.cpp"),
            (f"libs/yocto/yocto_trace.h", "contrib/libs/yocto/yocto_trace.h")
        ]
    )

def update_ufbx():
    update_target(
        "ufbx",
        "https://github.com/bqqbarbhg/ufbx.git",
        [
            (f"ufbx.h", "src/modules/voxelformat/external/ufbx.h"),
            (f"ufbx.c", "src/modules/voxelformat/external/ufbx.c")
        ]
    )

# Main function to run all updates
def main():
    update_emscripten_browser_file()
    update_stb()
    update_googletest()
    update_benchmark()
    update_imguizmo()
    update_implot()
    update_backward()
    update_im_neo_sequencer()
    update_dearimgui()
    update_glm()
    update_sdl2()
    update_tinygltf()
    update_tinyobjloader()
    update_simplecpp()
    update_simplexnoise()
    update_flextgl()
    update_libvxl()
    update_meshoptimizer()
    update_yocto()
    update_ufbx()

if __name__ == "__main__":
    main()