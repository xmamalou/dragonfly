# Revision history for the Dragonfly engine

## 0.0.1 -- 2022-7-5

Set-up version of Dragonfly. The build system (Cabal) has been set up and the first function has been added.
Baby steps.

## 0.0.1.1 -- 2022-7-7

* New datatypes:
    * Added DflWindow (Haskell only)
    * Added DflVector (C only)

* New functions:
    * Added dflVulkInstanceMaking (C only)
    * Added dflVulkLayersMaking (C only)
    * Added dflVulkValLayersVerifying (C only)
    * Added dflVulkExtensionSupplying (C only)

* Added documentation system using Doxygen
* Reordered Haskell interface
* Added DFL_NO_DEBUG flag for Vulkan Validation Layers (must be entered manually)
