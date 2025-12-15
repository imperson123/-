# T-MAC DevEco Wrapper

This folder is a minimal Harmony/DevEco wrapper to allow opening the repository
in DevEco Studio / Harmony Studio. It provides a small `module.json5`,
`oh-package.json5`, `build-profile.json5` and a minimal `entry` ability so the
IDE recognizes the project as a Harmony app.

How to use

- Open DevEco Studio and choose `Open Project`, then select the `deveco_project`
  directory inside this repository.
- If DevEco still warns about project type, try selecting the `deveco_project` folder
  explicitly (not the repository root).
- After opening, you can adjust `module.json5` fields (like `name`/`appName`) to
  match your expected package name.

Notes

- This wrapper is intentionally minimal; it is meant for editing/opening the
  repository inside DevEco. It does not change the main repository sources.
- If your DevEco version requires different files (e.g., specific `project` or
  framework files), tell me the DevEco version and I will adapt the wrapper.
