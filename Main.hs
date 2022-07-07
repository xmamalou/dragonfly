-- This is a dummy file. It's not part of the library. All library related files are 
-- in the Dragonfly/ and c/ directories.

import Dragonfly.Engine.Render 
import Control.Concurrent

window = DflSmallWindowtype "Test" 1080 720

main = do
    x <- dflWindowIniting window
    y <- dflVulkanIniting
    
    print x
    print y

    threadDelay 1000000
    
    dflVulkanKilling
    dflWindowKilling