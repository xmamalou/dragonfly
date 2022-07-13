-- This is a dummy file. It's not part of the library. All library related files are 
-- in the Dragonfly/ and c/ directories.

import Dragonfly.Engine.Render 
import Control.Concurrent

window = DflSmallWindow "Test" 1080 720
display = dflDisplay 0

main = do
    x <- dflRenderIniting window display 
    
    print x

    threadDelay 1000000
    
    dflRenderKilling window 