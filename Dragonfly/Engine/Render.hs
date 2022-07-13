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

-- DATATYPES
data DflWindow = DflSmallWindow {
    name  :: String, 
    width :: CInt, 
    height:: CInt
    }
                | DflFullWindow {
    name  :: String
    }
                | DflNoWindow 
    deriving (Eq)

data DflDisplay = DflDisplay {
    dimensions :: [CInt],
    rate       :: CInt
}

-- -----------------
-- WINDOW FUNCTIONS
-- -----------------

-- dflWindowIniting :
-- creates a window as defined by the DldflfWindow data type
-- 
-- PARAMS
-- window -> The window to be created. If DflSmallWindow, then
-- we have windowed mode. If DflFullWindow, then we have fullscreen
-- mode
--
-- RETURNS 0 for success, natural otherwise.
--
-- SINCE Dragonfly 0.0.1
dflWindowIniting :: DflWindow -> DflDisplay -> IO Int
dflWindowIniting window (DflDisplay [wid, hgt] rate) = do
    case window of
        (DflSmallWindow name width height) -> withCString name $ \windowname -> do
                                                c_dflWindowIniting windowname width height 0
        (DflFullWindow name) -> withCString name $ \windowname -> do
                                    c_dflWindowIniting windowname wid hgt 1

-- dflWindowKilling:
-- kills the window
--
-- SINCE Dragonfly 0.0.1
dflWindowKilling :: IO ()
dflWindowKilling = do
    c_dflWindowKilling

-- -----------------
-- DISPLAY FUNCTIONS
-- -----------------

-- dflDisplayInfoGetting
-- get some information about a display
--
-- PARAMS
-- display -> from which display to get the info from
-- info -> what to get. 0: width, 1: height, 2: refresh rate
--
-- RETURNS requested info, on success.
--
-- SINCE Dragonfly 0.0.4
dflDisplayInfoGetting :: CInt -> CInt -> CInt
dflDisplayInfoGetting display info = do
    c_dflDisplayInfoGetting display info

-- dflDisplay
-- wraps dflDisplayInfoGetting(display, info) for values info = {0, 1, 2} into a single
-- function that returns a DflDisplay object
--
-- PARAMS
-- display -> which display to analyse
--
-- RETURNS a DflDisplay variable
--
-- SINCE Dragonfly 0.0.4
dflDisplay :: CInt -> DflDisplay
dflDisplay display = do
    DflDisplay [(dflDisplayInfoGetting display 0), (dflDisplayInfoGetting display 1)] (dflDisplayInfoGetting display 2)

-- -----------------
-- VULKAN FUNCTIONS
-- -----------------

-- dflVulkanIniting:
-- initializes vulkan
--
-- PARAMS
-- onscreen -> Should allow window-related processes to go on for onscreen rendering?
--
-- RETURNS 0 for success, natural otherwise.
--
-- SINCE Dragonfly 0.0.1
dflVulkanIniting :: CInt -> IO Int 
dflVulkanIniting onscreen = do
    c_dflVulkanIniting onscreen

-- dflVulkanKilling
-- frees Vulkan resources
--
-- SINCE Dragonfly 0.0.1
dflVulkanKilling :: IO ()
dflVulkanKilling = do
    c_dflWindowKilling

-- -----------------
-- RENDER FUNCTIONS
-- -----------------

-- dflRenderIniting
-- Initializes the Renderer
-- 
-- PARAMS
-- window -> The window to create (if window = DflNoWindow, off screen rendering will be used)
-- display -> The display parameters
--
-- RETURNS 0 on success, natural number otherwise
--
-- SINCE Dragonfly 0.0.4
dflRenderIniting :: DflWindow -> DflDisplay -> IO Int
dflRenderIniting window display = do
    case window of 
        DflNoWindow -> dflVulkanIniting 0
        otherwise -> do
            dflWindowIniting window display
            dflVulkanIniting 1

-- dflRenderKilling
-- Kills the Renderer
--
-- PARAMS
-- window -> Sees if offscreen rendering was used
--
-- SINCE Dragonfly 0.0.4
dflRenderKilling :: DflWindow -> IO ()
dflRenderKilling window = do
    case window of
        DflNoWindow -> dflVulkanKilling
        otherwise -> do
            dflVulkanKilling
            dflWindowKilling
