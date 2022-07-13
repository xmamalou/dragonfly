# Revision history for the Dragonfly engine

## 0.0.1 (pre-a001) -- 2022-7-5

Set-up version of Dragonfly. The build system (Cabal) has been set up and the first function has been added.
Baby steps.

## 0.0.2 (pre-a002) -- 2022-7-7

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

## 0.0.3 (pre-a003) -- 2022-7-10

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

* Moved static functions' and variables' declarations from header to source

## 0.0.4 (pre-a004) -- 2022-7-13

* Fixed issue with repository
* The C source is now situated in the Dragonfly directory.
* Datatypes:
    * Added DflNoWindow subtype to DflWindow (Haskell)
    * Added DflDisplay type (Haskell)

* Variables:
    * Added Display related variables (C):
        * dflDisplayCount
        * dflWidth
        * dflHeight
        * dflRefreshRate
    * Added dflPresentQueue (C)

* Functions:
    * Added dflDisplayInfoGetting (C, Haskell)
    * Added dflDisplay (Haskell)
    * Added dflRender* functions:
        * dflRenderIniting
        * dflRenderKilling

* Processes
    * Added Surface initialization
