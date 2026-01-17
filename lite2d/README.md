# lite2d

## Getting Started

Below have to be installed on your system:
- OpenGL
- GLFW
- GLM

```
mkdir build
cd build
cmake ..
make
```

```sh
./lite2d -m (.moc3.json file) -r (.moc3.render-settings.json file) -t (texture file)

# Example:
./lite2d -m ../live2d-assets/mao_pro/mao_pro.moc3.json -r ../live2d-assets/mao_pro/mao_pro.moc3.render-settings.json -t ../live2d-assets/mao_pro/mao_pro.4096/texture_00.png
```
