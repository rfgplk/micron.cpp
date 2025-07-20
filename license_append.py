import os

license = b"//  Copyright (c) 2024- David Lucius Severus\n//\n//  Distributed under the Boost Software License, Version 1.0.\n//  See accompanying file LICENSE_1_0.txt or copy at\n//  http://www.boost.org/LICENSE_1_0.txt\n"
exts = [ ".cpp", ".hpp", ".h", ".c", ".hh", ".cc"]
for root, _, files in os.walk(".", topdown=True, followlinks=False):
    for name in files:
        if any(name.lower().endswith(ext) for ext in exts):
            path = os.path.join(root, name)
            print("Appending to: " + path)
            try:
                with open(path, "rb") as f:
                    content = f.read()
                with open(path, "wb") as f:
                    f.write(license + content)
            except Exception:
                pass
