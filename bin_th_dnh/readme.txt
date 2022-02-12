●	Before We Get Started
		This software is a freeware.
		The creators are not liable for any mental, physical, or emotional damages caused by using the application.

●	Installation
　		1. Use a software that is able to extract .zip files. 
			If you're seeing this you've probably already done this step. Good job.
		2. Select the path to which to extract the files.
		3. Yeah.
		4. Suffer or enjoy, I don't know.
		
●	Notes
		■	Running on Wine
				If the engine shows an error regarding shader compilation on startup, it's likely because of Wine failing to load required dlls.
				
				Steps:
					1a) Install winetricks if you haven't yet.
					2a) Run 'winetricks d3dx9 d3dcompiler_43' at the console.
					
					1b) If you've performed the above steps, and the engine still does not work, continue below.
					2b) Run winetricks -> "Select the default wineprefix" -> "Run winecfg"
					3b) Select either "Default Settings", or "Add application..." and select the ph3sx binary.
							- For "Windows Version", select at least Windows 7.
					4b) Go to the "Libraries" tab, and add overrides for the following dlls:
							- d3dx9_43
							- d3dcompiler_43
					5b) Apply the settings, and everything should now work.

●	Uninstallation
　		Simple delete all the related files.
		This application does not utilize registry keys nor AppData storage.

●	Key Controls (default)
		Arrow Keys:	Move
		Z:			Shoot, Select
		X:			Bomb, Cancel
		Left Shift:	Hold for Focused Movement
		Left Ctrl:	Hold for Fast Mode
		R:			Return to Script Select Screen (Unavailable by default in package scripts)
		Backspace:	Restart Script  (Unavailable by default in package scripts)
		
		** Default key controls are purely engine-default, and may be overridden and changed from script to script.

Check out the sample scripts for demonstrations of new functions and features.



REPORT BUGS AND/OR GIVE FEATURE REQUESTS TO:
	
	●	https://github.com/Natashi/Touhou-Danmakufu-ph3sx-2/issues on GitHub
	
	●	https://discord.gg/f9KFujKGEx on the ph3sx development Discord server