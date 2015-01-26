#Continuum Screen Bot

##Requirements
- Opaque large radar.  
- Windowed mode. 640x480 and 800x600 only.  
- Constant window focus.  
- Default brightness settings.  
- Default keybindings.  
- Disable rolling ships

##Running
- Change the Include variable in bot.conf to point to the file with the config settings
- Change the Level variable in the config file
- Make sure Multi-Fire Starts On is checked in View -> Options -> Other if you want multi-fire.  
- Sometimes the keys will stick after closing the bot. Just press left, right, up, down, and control to fix it.  
- Put (public) in the zone folder if you want to use a turret with the bot.  
- The bot will need to be restarted if you change items that adjust energy.  
- You can run the bot in a virtual machine if you want to be able to still use the computer normally.  

### Config
- Add ?log bot.log to auto.
- Create /?attach macro and set it to F7 keybinding.   
- Update the config settings. At least Level, MapZoom, LogFile, and Name  

##Compiling
- It will probably only compile with Visual Studio 2013.  
- Add _USE_MATH_DEFINES;NOMINMAX to preprocessor definitions.  
