open folder HW5/out/build/x64-Debug , and copy diamond.obj and texture.bmp to it . Then run the program.

if you want to remove the background press - multiple times to remove the background

for testing check Show Triangle Fill to be ON 

for Part 1 :

Shading Mode: Set to "Flat" (it doesn't matter for ambient, but Flat is simplest).

Light Diffuse & Material Diffuse: Set these to (0,0,0) (temporarily turn off diffuse so you only see ambient).

Settings to use:

Light Ambient: (0.5, 0.0, 0.0) (Red light)

Material Ambient: (1.0, 1.0, 1.0) (White surface)

Part 2: 

Shading Mode: Set to "Flat".

Settings to use:

Light Diffuse: (1.0, 1.0, 1.0) (White light)

Material Diffuse: (1.0, 0.0, 0.0) (Red surface)

Light Ambient & Material Ambient: Set to (0.1, 0.1, 0.1) (Very low, so you can see the dark faces).

Light Position: Put the light at (200, 200, 200) (Above and to the right).

Part3:

Shading Mode: Set to "Flat" (or Phong, but Flat is fine for proving the math works).

Settings to use:

Light Diffuse: (0.5, 0.5, 0.5) (Dim it a bit so the highlight pops).

Light Specular: (1.0, 1.0, 1.0) (White highlight).

Material Specular: (1.0, 1.0, 1.0) (White reflection).

Shininess: Set to 32 (Medium).

Light Position: Put the light at (100, 100, 300) (Directly in front and to the right).

Part 4:

Shading Mode: Set to "Flat". Take a quick look at the cube.

Shading Mode: Switch to "Phong".

Settings to use:

Light Diffuse: (1.0, 1.0, 1.0).

Material Diffuse: (1.0, 0.0, 0.0) (Red).

Shininess: 32.

Part 5.1:

Shading Mode: Set to "Phong" or "Flat".

Settings to use:

Check the box "Show Debug Vectors".

Move the Light Position so it's clearly off to one side (e.g., (200, 100, 100)).


Part 5.2:

Shading Mode: Set to "Phong" 

Settings to use:

Check the box "Use Texture".

Material Diffuse: Set to (1.0, 1.0, 1.0) (Important: If this is (0,0,0), the texture will be black!).

Light Ambient: (0.2, 0.2, 0.2).

Light Diffuse: (1.0, 1.0, 1.0).

Keep Material Specular at (1,1,1) so you still have a shiny highlight on the texture.

change the position of light as you like to see the effect of light on the texture.
