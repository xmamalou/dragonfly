module Dragonfly.Render where
-- Dragonfly.[Name] modules offer a nicer and more Haskell
-- friendly interface for the c functions

import Dragonfly.Bind.Render

import Foreign.C.Types
import Foreign.C.String

-- dlfWindowIniting :
-- creates a window
-- windowName -> name of window
-- (width, height) -> dimensions of window
-- fullscreen -> ignore above dimensions and set window fullscreen, if 1.
dlfWindowIniting :: String -> CInt -> CInt -> CInt -> IO Int
dlfWindowIniting windowName width height fullscreen = do
    withCString windowName $ \cstr ->
        c_dlfWindowIniting cstr width height fullscreen