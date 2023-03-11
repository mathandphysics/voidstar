# `void*`
## A Real-Time Black Hole Ray Tracer


### About

`void*` is a real-time black hole ray tracer.  You can fly around and observe a black hole with accretion disk from any angle.


### Screenshots

![Main shot](docs/images/mainshot.jpeg)

![Afar](docs/images/afar.jpeg)

![Closeup Debug](docs/images/closeup_debug.jpeg)

![Above Debug](docs/images/above_debug.jpeg)

![Without Lensing](docs/images/without_lensing.jpeg)


### Controls
Use `Esc` to close the menus in order to be able to move around.

```
W : Forward
S : Backward
A : Left
D : Right
Left Shift : Up
Spacebar : Down
Escape : Pause/Unpause
Right Click : Take Screenshot
Mouse Scroll : Change Movement Speed
```


### Compiling
#### Visual Studio + vcpkg

Use vcpkg to install the following:

`vcpkg install glfw3:x64-windows glfw3:x64-windows-static`

`vcpkg install imgui[core,opengl3-binding,glfw-binding,docking-experimental]:x64-windows imgui[core,opengl3-binding,glfw-binding,docking-experimental]:x64-windows-static`

`vcpkg install glm:x64-windows glm:x64-windows-static`

`vcpkg install glad[gl-api-46,loader]:x64-windows glad[gl-api-46,loader]:x64-windows-static`

And make sure you've already used `vcpkg integrate install` so that Visual Studio can immediately use the installed packages.

Then just open the solution file in Visual Studio and compile.


### Credits

- The skybox image of the Milky Way galaxy is thanks to the [European Space Agency](https://sci.esa.int/web/gaia/-/the-colour-of-the-sky-from-gaia-s-early-data-release-3-equirectangular-projection)

- The accretion disk texture is from [this](https://github.com/rantonels/starless) excellent non-realtime black hole raytracer by Riccardo Antonelli.