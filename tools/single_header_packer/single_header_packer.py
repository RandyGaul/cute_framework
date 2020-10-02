import fnmatch
import os.path
import sys

def print_help():
    print(
"""usage: python single_header_packer.py --macro <macro> --root <path> [--intro <your_intro.txt>] --hdrs <hdrs.txt> --srcs <srcs.txt>

       <macro> is the name of your library, such as CUTE_FRAMEWORK.
       <path> is an optional parameter specifying where all input source files are relative to (but not the intro).
       <your_intro.txt> is a text file containing the introduction text/documentation.
       <hdrs.txt> is a text file where each line is a header file. Order matters.
       <srcs.txt> is a text file where each line is a header or source file. Order matters.

       The resulting code is packed as follows:

           /*
           [your_intro.txt file contents]
           */

           #ifndef <macro>_SINGLE_FILE_HEADER
           #define <macro>_SINGLE_FILE_HEADER
           [header file contents from --hdrs]
           #endif /* <macro>_SINGLE_FILE_HEADER */

           #ifdef <macro>_IMPLEMENTATION
           [header and source file contents from --srcs]
           #endif /* <macro>_IMPLEMENTATION */
           """)

def parse_files(path):
    with open(path) as f:
        lines = [line.rstrip() for line in f]
    return lines

def omit_includes(str, files):
    for file in files:
        fname = os.path.basename(file)
        if ".h" in file:
            # First try to remove newlines.
            str = str.replace("#include \"" + fname + "\"\n", "");
            str = str.replace("#include <" + fname + ">\n", "");

            # Handle any other cases with odd whitespace usage.
            str = str.replace("#include \"" + fname + "\"", "");
            str = str.replace("#include <" + fname + ">", "");
    return str

# Main start.
# -----------

if len(sys.argv) < 2:
    print_help()
    exit()

root = ""
intro = []
hdrs = []
srcs = []
cur_arg = 1
macro = ""

# Parse args.
# -----------
while cur_arg < len(sys.argv):
    if sys.argv[cur_arg] == "--help":
        print_help()
        exit()
    elif sys.argv[cur_arg] == "--root":
        cur_arg += 1
        root = sys.argv[cur_arg]
    elif sys.argv[cur_arg] == "--macro":
        cur_arg += 1
        macro = sys.argv[cur_arg]
    elif sys.argv[cur_arg] == "--intro":
        cur_arg += 1
        intro = sys.argv[cur_arg]
    elif sys.argv[cur_arg] == "--hdrs":
        cur_arg += 1
        hdrs = parse_files(sys.argv[cur_arg])
    elif sys.argv[cur_arg] == "--srcs":
        cur_arg += 1
        srcs = parse_files(sys.argv[cur_arg])
    else:
        print("Unknown argument " + sys.argv[cur_arg])

    cur_arg += 1

if macro == "":
    print("Option --macro <macro> is mandatory")
    exit()

# Print concatenated output.
# --------------------------
print("/*")
sys.stdout.write(open(intro, 'r').read())
print("")
print("*/")
print("")

print("#ifndef " + macro + "_SINGLE_FILE_HEADER");
print("#define " + macro + "_SINGLE_FILE_HEADER");
print("")
for f in hdrs:
    if f != "":
        sys.stdout.write(omit_includes(open(os.path.join(root, f), 'r').read(), hdrs))
        print("")
print("#endif /* " + macro + "_SINGLE_FILE_HEADER */");
print("")

print("#ifdef " + macro + "_IMPLEMENTATION");
print("#infdef " + macro + "_IMPLEMENTATION_ONCE");
print("#define " + macro + "_IMPLEMENTATION_ONCE");
for f in srcs:
    if f != "":
        print(omit_includes(open(os.path.join(root, f), 'r').read(), hdrs + srcs))
        print("")
print("#endif /* " + macro + "_IMPLEMENTATION_ONCE */");
print("#endif /* " + macro + "_IMPLEMENTATION */");
