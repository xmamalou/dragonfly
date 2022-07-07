-- This is a dummy file. It's not part of the library. All library related files are 
-- in the Dragonfly/ and c/ directories.

import Dragonfly.Engine.Render 
import Control.Concurrent

window = DlfSmallWindowtype "Test" 1080 720

main = do
    x <- dlfWindowIniting window
    y <- dlfVulkanIniting
    
    print x
    print y

    threadDelay 1000000
    
    dlfVulkanKilling
    dlfWindowKilling