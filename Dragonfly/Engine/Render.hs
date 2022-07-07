module Dragonfly.Engine.Render where
-- Dragonfly.Engine.[Name] modules offer a nicer and more Haskell
-- friendly interface for the c functions

-- all functions defined in Dragonfly.Bind.[name] will be "redefined"
-- here, even if they don't need any argument translation

-- RENDER: 
import Dragonfly.Bind.Render

import Foreign.C.Types
import Foreign.C.String

data DlfWindow = DlfSmallWindowtype {
    wsName  :: String, 
    wsWidth :: CInt, 
    wsHeight:: CInt
    }
                | DlfFullWindowtype {
    wfName  :: String
    } 
    deriving (Eq)
    
-- dlfWindowIniting :
-- creates a window as defined by the DlfWindow data type
-- if window has only a String as an argument, it's inferred
-- that a fullscreen window is desired. If window also has
-- numbers (integers) as arguments, they will be interpreted
-- as the size of the window (NOT the resolution of the window).
-- The window itself will be in windowed mode
--
-- RETURNS error code in the form of an integer. 0 for success,
-- positive otherwise. Refer to documentation for exact error code 
dlfWindowIniting :: DlfWindow -> IO Int
dlfWindowIniting window = do
    case window of
        (DlfSmallWindowtype name width height) -> withCString name $ \windowname -> do
                                                    c_dlfWindowIniting windowname width height 0
        (DlfFullWindowtype name) -> withCString name $ \windowname -> do
                                        c_dlfWindowIniting windowname 2560 1440 1

-- dlfWindowKilling:
-- frees window resources
dlfWindowKilling :: IO ()
dlfWindowKilling = do
    c_dlfWindowKilling

-- dlfVulkanIniting:
-- initializes vulkan
--
-- RETURNS error code in the form of an integer. 0 for success,
-- positive otherwise. Refer to documentation for exact error code.
dlfVulkanIniting :: IO Int 
dlfVulkanIniting = do
    c_dlfVulkanIniting

dlfVulkanKilling :: IO ()
dlfVulkanKilling = do
    c_dlfWindowKilling