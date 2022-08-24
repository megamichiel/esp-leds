# Leds

This is the code for my led strip controller for esp32 microcontrollers (formerly for esp8266, although the code most likely also still works for esp8266, but the project structure is different, since this uses esp-idf rather than something like Arduino IDE).

This is one of my projects I put on GitHub just so I have a bit of a portfolio. I got carried away with writing this readme, and although the code is fairly readable in many cases due to the elaborate naming I use, comments are sparse so don't go blaming me if you don't understand the code.



## Features

### Formulas

This project uses a "formula" system to determine the hue/saturation/value of individual leds. These are strings that are sent to the device containing a formula that supports variables, so that animations can be created without having them to be pre-programmed on the controller. There are three variables:

- **N** is the number of leds on the strip

- **x** is the index of the led, in range [0, N-1]
- **t** is the current tick

What are ticks, you ask? I stole this concept from Minecraft (although they might have taken it from something else as well, it's a pretty generic thing). A tick is a 50-millisecond interval (although if you're feeling adventurous, you could change that, it's configurable, called TICK_DURATION in includes.h), so after 1 second, 20 ticks have passed. If any of the active formulas contains the *t* variable, leds are updated every tick, and that's how you can create animations. It always starts at 0 on startup, and increases until it's reached 0x7FFFFFFF, then goes back to 0. It never goes below 0, so you don't have to worry about that.

The hue/saturation/value (I'll call them h,s,v) all range from 0 to 255. This corresponds with what the FastLED library uses. There's currently a [Pixel Reference](https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors) page on their github which explains things well. If that page doesn't exist anymore, I challenge you to search the FastLED documentation yourself, and if that doesn't exist anymore, well darn. H should be self-explanatory, S = 0 means white (no color), S = 255 means as little white as possible (so with H = 0, the color is as red as can be), V = 0 means no brightness, V = 255 means max brightness (V = 0 is black, but there's no black on lights, just reduced brightness).

If h/s/v values are specified outside the [0, 255] range, the following is done:

- H is taken MOD 256, so H = 260 becomes H = 4
- S and V are clamped in range [0, 255], so S/V = 260 becomes S/V = 255



This formula system can be a bit difficult if you're not familiar with it, but it comes down to simple calculations that you'd normally do in programming/middle school.

Near the top of formula.cpp is a constant called "op_sym", which contains all of the symbols that can be used. There's:

- The typical operators, +-*/%^
- Absolute, e.g. **|-5|** = 5, **|5|** = 5
- You can use parantheses to create precedence, for instance **(3 + 2) * 2** = **5 * 2** = 10
- **max** and **min** which takes the max/min of various values, e.g. **5 max 6** = 6, **5 min 6** = min, **3 min 4 min 5** = 3
- Some conditional operators, which either produce a 1 or a 0: =, >=, <=, >, <. For instance **4 <= 5** returns 1, since 4 is less than or equal to 5.
- The ternary conditional ?:, for instance **x > 100 ? 255 : 128**, which becomes 255 when x (the current led's index) is larger than 100, and 128 otherwise.

Regarding precedence, it follows standard C rules, with **max** and **min** between +- and the equality testers. In the op_sym variable I referenced above, the precedence is determined by the line they are on in the file (although the code uses the op_lvl variable above it, where you can see the value of the operators' precedence).



You can for instance create:

- hue = **t** makes all leds the same, and cycles through all the hues and then comes back around
- hue = **x** creates a stationary rainbow since every led has a hue 1 higher than the previous
- hue = **x + t** creates a moving rainbow effect, which cycles through the leds since **t** updates this value every tick.

You can get very creative with this.



#### Fade

To improve performance with complex formulas, I included a "fade" value that can be configured by a client as well, like the formulas. With a default value of 1 it does nothing, but its purpose is to reduce the number of formula calculations. With a value of 2, leds with indices a multiple of 2 are only calculated (0, 2, 4, etc.) and for the leds in between, their value is the average of the leds around it. For instance, if led 0 is red and led 2 is green, then led 1 will be yellow-ish. With values higher than 2, it becomes a gradual shift. With 3 for instance, is led 0 is red and led 3 is green, then led 1 is 67% red and 33% green, and led 2 is 33% red and 67% green.



### Multiple led strips

In my home, in each room I have two led strips connected to the controller: one moving along the wall on one side of the room, the other along the other wall. This project treats these strips as one long led strip, so make my life easy. In includes.h there's a few things you need to specify:

- A definition called NUM_LEDS, where you can specify the total number of leds
- Right below it is led_count, which is an int array, where you can specify the count of each of the led strips
- Right below that is led_strips, which specifies the number of led strips.
- Below that are the led strip type and color order. The code assumes you have the same led strip types connected to a single esp. If you don't, you need to adjust the FastLED.addLeds<...> lines a bit that I mention below.
- Below that are the pins for strips 1 and 2. If you for some reason have more strips connected, you could add defines here.

- Unfortunately, I could not find a dynamic way to initialize the led strips, so if you want to increase/decrease the number of strips, you need to go to leds.cpp, scroll down to the setup() function (around line 200 at the time of writing), and find the lines that say FastLED.addLeds<...>. Add/remove lines as needed, and make sure that their offsets are correct. For the third strip, you'd need offset "leds + led_count[0] + led_count[1]" and count led_count[2]. It's not pretty per se, but sue me, this code is run once on startup so I don't care.



### Communication

Uses a tiny, insecure (I only need to communicate with this device when I'm home, so it's connected to my router and I don't want to bother with making it more complex than it needs to be), protocol, to communicate with devices.

In includes.h there's a bunch of definitions at the bottom where you can specify the router's SSID and password, as well as the port the device should run its server on.

The device also enables MDNS with a name specified in the same place as the other network stuff. It never really seemed to work for me though, so don't blame me if you can't get it to work, I won't fix it for you. I think I ran into the issue of Android not supporting MDNS well, but it's a long time ago so I don't remember it that well.



### Other things you should know

- It automatically tries to reconnect to wifi if it loses connection.
- Stuff is saved, so you can safely restart the esp without it resetting everything.
  - Keep in mind that data is only saved if stuff has been modified, and 5 seconds have passed without any modifications. This is to reduce the number of writes to storage. So if you restart the esp within 5 seconds after something has been changed, there's a good chance that modification is lost.
- To improve performance:
  - If no formulas contain **t**, the leds are only updated once.
  - If no formulas contain **x**, the value is computed once and then reused for all leds.
  - If there's no active connection (aka no packet has been received for the past 10 seconds), packets are only checked every second, so it won't always respond immediately.
  - If there's no connection and the brightness is at 0 (so the light is off), ticks change from 20 times per second to once every second since there's nothing to do.
- Although the formula system makes it easy to create new led strip configurations without having to upload new code, the calculation of formulas is slower than using native C code, so if you're using complex formulas, the controller can take longer than a tick takes to compute formulas (or it's at least straining on the controller if it's on for a long time). Keep that in mind and try to be nice to your esp.



### Clients

I made several apps and one web app to communicate with the controller. Some of them can be found in my public repositories. If you feel like making your own, the protocol is very simple:

1. Connect to the controller using UDP
2. As long as you're connected, you can send packets:
   - Start with a 2-byte big endian number containing the length of the following packet (including packet ID)
   
   - Then the packet ID, which is either 0, 1, or 2
   
     - 0 is a ping packet, which simply sends the same packet back (0x00:0x01:0x00). This can be used to spam the local network's ip addresses to find connected led strips, if you want to get fancy.
   
     - 1 is an update packet, which starts with a flag byte, where specific enabled bits specify the values that are updated. If the bit for a value is enabled, it's included in the packet after the flag in the following order:
   
       - Bit 0 = brightness, which is a single byte.
       - Bit 1 = fade (described earlier in the Formulas section), which is a 2-byte big endian number
       - Bits 2, 3, and 4 = hue, sat, val, which consist of:
         - A boolean which specifies if the formula is a double formula
         - A 0-terminated string (so the string in bytes, followed by a 0 (not the '0' character, an actual value 0))
   
       The client is then sent 0x00:0x01:0x01 (which is an empty packet with id 1)
   
     - 2 is a status request packet, which retrieves the current state of the strip(s). It sends a packet with ID 2 back, which first contains the number of leds as a 2-byte big endian number, and then, in the same order as above, all the values that can be updated. It doesn't include the flag byte, so it just contains brightness, fade, and hsv.



## Setup

This project uses the [esp-idf](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) framework, so set that up if you want to use it. It might be easier to just try to turn this into an Arduino IDE project if you're not familiar with esp-idf. For that, you should rename leds.cpp to leds.ino and treat that as the main file, then copy that along with all the .h and .cpp files into your Arduino project.

If you decide to use esp-idf, make sure to include espressif's [arduino-esp32](https://github.com/espressif/arduino-esp32) component in the "components" folder (if that folder doesn't exist, create it).

This project uses [FastLED](https://github.com/FastLED/FastLED) to control the leds, so make sure that's installed.

Through the documentation above there are references to .h and .cpp files which describe the neccesary changes needed to make it work for your scenario. Search on this page (probably CTRL/CMD+f) if you want to find them, I'm not gonna write them again. It's mostly in includes.h though.

