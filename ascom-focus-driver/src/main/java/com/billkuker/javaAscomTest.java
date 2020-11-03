package com.billkuker;
/*
#####################################################################################
#####                                                                           #####
#####                    Java ASCOM example telescope control                   #####
#####                           using Jacob jacob-1.19                          #####
#####  https://sourceforge.net/projects/jacob-project/files/jacob-project/1.19/ #####
#####                                                                           #####
#####              NOT INTENDED TO BE USED WITH A REAL TELESCOPE!               #####
#####                 USE THE ASCOM TELESCOPE SIMULATOR INSTEAD                 #####
#####                         FOR TESTING PURPOSES ONLY                         #####
#####                                                                           #####
#####       This app developed by Rick B. and N. de Hilster, April 2020         #####
#####                                                                           #####
#####################################################################################
*/

import com.jacob.activeX.*;
import com.jacob.com.*;
import java.io.IOException;


public class javaAscomTest {
  public static void main(String args[]) {
		new javaAscomTest();
  }

	public javaAscomTest()
	{
		// for scope commands, see: https://ascom-standards.org/Help/Developer/html/T_ASCOM_DriverAccess_Telescope.htm
		Dispatch theScope;
		Variant tracking;
		Variant slewing;
		Variant parked;
		Variant canSetTracking;
		Variant siteLat;
		Variant siteLon;
		Variant sidTime;
		Variant ra;
		Variant dec;
		Variant	connected;
		double slewRA;
		double slewDec;
		boolean isConnected = false;
		boolean isTracking;
		boolean isParked = false;
		int setTrackingCycle = 10;
		int startSlewCycle = 20;
		int resetLoopCycle = 50;
		
		// assuming this code is started from a CMD-terminal, the terminal is cleared
		clearConsole();
		
		// In C#: using ASCOM.Utilities;
		// In C#: Util util = new Util();
		// In C#: string version = util.PlatformVersion;
		Dispatch util = new ActiveXComponent( "ASCOM.Utilities.Util");
		String version = Dispatch.call( util, "PlatformVersion").toString();
		System.out.println( "The ASCOM Platform Version is " + version);
		
		// In C#: using ASCOM.Utilities;
		// In C#: chooser chooser = new Chooser();
		// In C#: chooser.DeviceType = "Telescope";
		// In C#: string scopeID = chooser.Choose( "ASCOM.Simulator.Telescope" );
		ActiveXComponent chooser = new ActiveXComponent("ASCOM.Utilities.Chooser");
		chooser.setProperty("DeviceType", "Telescope");
		String scopeId = Dispatch.call( chooser, "Choose", "ASCOM.Simulator.Telescope").toString();
		System.out.println( "You selected this driver - " + scopeId +".");

		if (!scopeId.equals("")) // if Cancel was pressed in the Chooser it stops here
		{
			// actually connect to the scope
			theScope = new ActiveXComponent(scopeId);
			Dispatch.put(theScope, "Connected", true);
			connected = Dispatch.call(theScope, "Connected");
			isConnected = (""+connected).equals("true");

			// allow for 3 seconds to show the user which scope was selected before clearing the terminal again
			int i=3;
			while (i>0)
			{
				System.out.println( "Starting scope in "+i+" seconds ");
				i--;
				try
				{
					Thread.sleep(1000);
				} catch (InterruptedException IE) {}
			}
			clearConsole();

			if (isConnected) // if no connection could be made to the scope it stops here
			{
				// start with a non-tracking mount, just to show that this can be done
				Dispatch.put(theScope,"Tracking", false);
				// here is the main loop
				while (!isParked) // main loop exits when user parks the scope
				{
					// after enough cycles set the mount in tracking mode, test if that was succesful
					if (i==setTrackingCycle) Dispatch.put(theScope,"Tracking", true);
					tracking = Dispatch.call(theScope, "Tracking");
					isTracking = (""+tracking).equals("true");
					
					// get all kind of info from the mount
					slewing = Dispatch.call(theScope, "Slewing"); // check if the scope is slewing
					parked = Dispatch.call(theScope, "AtPark"); // check if the scope is parked
					isParked = (""+parked).equals("true");
					if ((i==0)&&isParked) // if it is parked at start-up, unpark it for this demo to allow to run
					{
						Dispatch.call(theScope, "UnPark");
						isParked = false;
					}
					canSetTracking = Dispatch.call(theScope, "CanSetTracking"); // check if the scope can be set tracking
					siteLat = Dispatch.call(theScope, "SiteLatitude"); // retrieve site latitude, normally only required once, but now in the loop for the user to play with
					siteLon = Dispatch.call(theScope, "SiteLongitude"); // retrieve site longitude, normally only required once, but now in the loop for the user to play with
					sidTime = Dispatch.call(theScope, "SiderealTime"); // get the mount's sidereal time
					ra = Dispatch.call(theScope, "RightAscension"); // get the current RA
					dec = Dispatch.call(theScope, "Declination"); // get the current DEC
					
					// after even more cycles it is time to slew, unless the mount is parked or not tracking
					// Giving the slew command is best done in a separate thread
					if ((i==startSlewCycle) && isTracking && !isParked)
					{
						slewRA = Double.parseDouble(""+ra) + Math.random()*2; // calculate a RA position to slew to
						slewDec = Double.parseDouble(""+dec)+ Math.random()*2; // calculate a DEC position to slew to
						Dispatch.put(theScope,"TargetRightAscension", slewRA); // set the slew RA
						Dispatch.put(theScope,"TargetDeclination", slewDec); // set the slew DEC
						Dispatch.call(theScope,"SlewToTarget"); // tell the mount to slew
					}

					i++; // update the cycle counter
					// display all acquired info to the user
					System.out.println("Cycle: "+i);
					System.out.println("Tracking: "+tracking);
					System.out.println("Slewing: "+slewing);
					System.out.println("Can set tracking: "+canSetTracking);
					System.out.println("Mount latitude: "+siteLat);
					System.out.println("Mount longitude "+siteLon);
					System.out.println("Sidereal time: "+sidTime);
					System.out.println("Right ascension: "+ra);
					System.out.println("Declination: "+dec);
					
					// wait a bit before polling again
					try
					{
						Thread.sleep(500); 
					} catch (InterruptedException IE) {}
						
					// then clear the console
					clearConsole();
					
					// after again more cycles reset the counter to zero
					if (i==resetLoopCycle) i=0;
				}
				if (isParked) System.out.println("The scope is parked, goodnight!");
			}
			else
				System.out.println("Not connected!");
		}
		else
			System.out.println("No scope selected!");
	}

	public final static void clearConsole()
	{
	     //Clears Screen in java
	    try {
	        if (System.getProperty("os.name").contains("Windows"))
	            new ProcessBuilder("cmd", "/c", "cls").inheritIO().start().waitFor();
	        else
	            Runtime.getRuntime().exec("clear");
	    } catch (IOException | InterruptedException ex) {}
	}

}
