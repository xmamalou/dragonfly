module Dragonfly.Bind.Render where

-- ERROR CODES
--{-# LANGUAGE CPP #-}
 
-- #define DLF_SUCCESS 0

-- #define DLF_SDLV_INIT_ERROR 1 // Couldn't initialize SDL (video)
-- #define DLF_SDL_WINDOW_INIT_ERROR 2 // Couldn't initialize SDL Window 

--

{-# LANGUAGE ForeignFunctionInterface #-}

import Foreign
import Foreign.C.Types
import Foreign.C.String

foreign import ccall "../../c/include/render.h dlfWindowIniting"
    c_dlfWindowIniting :: CString -> CInt -> CInt -> CInt -> IO Int