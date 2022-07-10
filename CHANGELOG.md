# Revision history for the Dragonfly engine

## 0.0.1.0 -- 2022-7-5

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

## 0.0.1.2 -- 2022-7-10

* Datatypes:
    * Renamed old DflVector to DflSet (C)
    * Added DflVector (C only)
    * Renamed DflSmallWindowtype and DflFullWindowtype to DflSmallWindow and DflFullWindow respectively (Haskell)

* Variables:
    * Added dflRealGPU object (C only, currently)
    * Added dflGPU object (C only, currently)
    * Added queues:
        * dflGraphicsQueue (C only, currently)
        * dlfComputeQueue (C only, currently)

* Fixed error where Vulkan couldn't find Validation Layers

* New processes:
    * Added Debug Messenger initialization (use DFL_NO_DEBUG flag to disable)
    * Added Physical Device Initialization
    * Added Logical Device Initialization
    * Added Queue Initialization

* Moved static functions' and variables' declarations from header to source.
