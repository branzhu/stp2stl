# Third-Party Notices

This repository includes third-party software. The licenses below apply to the third-party components only, not to the `stp2stl` code in this repository.

## Open CASCADE Technology (OCCT)

- Component: Open CASCADE Technology (OCCT)
- Upstream: Open CASCADE SAS
- Version: 7.9.3 (this project expects tag `V7_9_3` in `third_party/occt`)
- Location: `third_party/occt`
- License: GNU Lesser General Public License v2.1 (LGPL-2.1) with the Open CASCADE exception
- License texts:
  - `licenses/LICENSE_LGPL_21.txt`
  - `licenses/OCCT_LGPL_EXCEPTION.txt`

### Notes for binary redistribution (static linking)

This project may build OCCT as static libraries and link them into `stp2stl` binaries. If you redistribute binaries that include OCCT (statically or dynamically linked), you must comply with OCCT's license terms.

At a minimum, make sure you:

- Provide prominent notice that your binaries use OCCT.
- Include the OCCT license texts listed above.
- Provide (or offer access to) the corresponding OCCT source code and any modifications you made to it.
- Ensure recipients can replace/modify the LGPL-covered library portion as required by LGPL-2.1.

This file is provided for convenience and does not replace the actual license texts.
