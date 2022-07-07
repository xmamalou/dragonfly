-- Copyright 2022 Christopher-Marios Mamaloukas
--
-- Redistribution and use in source and binary forms, with or without modification, 
-- are permitted provided that the following conditions are met:
--
-- 1. Redistributions of source code must retain the above copyright notice, 
-- this list of conditions and the following disclaimer.
--
-- 2. Redistributions in binary form must reproduce the above copyright notice, 
-- this list of conditions and the following disclaimer in the documentation and/or other materials 
-- provided with the distribution.
--
-- 3. Neither the name of the copyright holder nor the names of its contributors 
-- may be used to endorse or promote products derived from this software without 
-- specific prior written permission.
--
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
-- THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
-- IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
-- SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
-- PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
-- HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
-- OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
-- EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

module Dragonfly.Engine.Render where
-- Dragonfly.Engine.[Name] modules offer a nicer and more Haskell
-- friendly interface for the c functions

-- all functions defined in Dragonfly.Bind.[name] will be "redefined"
-- here, even if they don't need any argument translation

-- RENDER: 
import Dragonfly.Bind.Render

import Foreign.C.Types
import Foreign.C.String

data DflWindow = DflSmallWindowtype {
    wsName  :: String, 
    wsWidth :: CInt, 
    wsHeight:: CInt
    }
                | DflFullWindowtype {
    wfName  :: String
    } 
    deriving (Eq)
    
-- dflWindowIniting :
-- creates a window as defined by the DldflfWindow data type
-- if window has only a String as an argument, it's inferred
-- that a fullscreen window is desired. If window also has
-- numbers (integers) as arguments, they will be interpreted
-- as the size of the window (NOT the resolution of the window).
-- The window itself will be in windowed mode
--
-- RETURNS error code in the form of an integer. 0 for success,
-- positive otherwise. Refer to documentation for exact error code 
dflWindowIniting :: DflWindow -> IO Int
dflWindowIniting window = do
    case window of
        (DflSmallWindowtype name width height) -> withCString name $ \windowname -> do
                                                    c_dflWindowIniting windowname width height 0
        (DflFullWindowtype name) -> withCString name $ \windowname -> do
                                        c_dflWindowIniting windowname 2560 1440 1

-- dflWindowKilling:
-- frees window resources
dflWindowKilling :: IO ()
dflWindowKilling = do
    c_dflWindowKilling

-- dflVulkanIniting:
-- initializes vulkan
--
-- RETURNS error code in the form of an integer. 0 for success,
-- positive otherwise. Refer to documentation for exact error code.
dflVulkanIniting :: IO Int 
dflVulkanIniting = do
    c_dflVulkanIniting

dflVulkanKilling :: IO ()
dflVulkanKilling = do
    c_dflWindowKilling