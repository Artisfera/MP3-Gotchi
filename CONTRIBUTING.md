# Contributing

Thanks for treating MP3-Gotchi as a community project.

## Before Editing Firmware

Use the latest sketch in `firmware/src/`.

For user-facing firmware changes, start with `UserConfig.h`.

Avoid editing old folders in `firmware/reversions/` unless you are documenting or comparing historical behavior.

## Keep Commits Clean

Do not commit:

- local `.venv` folders,
- `.backup` folders,
- Arduino build outputs,
- Python `__pycache__`,
- real songs or other copyrighted audio files,
- temporary files from editors or operating systems.

## Firmware Version Rules

When publishing a new firmware version:

1. Copy the current latest sketch to a new matching folder name.
2. Rename the `.ino` file to match the folder.
3. Update version strings in the sketch README, `.ino` header, and boot banner.
4. Keep `firmware/src/` as the latest sketch only.
5. Copy the same release to `firmware/reversions/`.

## License

Contributions are accepted only under the project non-commercial terms.

By submitting a contribution, you agree that it may be distributed with MP3-Gotchi under the repository `LICENSE`, unless a different written agreement is made before the contribution is accepted.

Commercial use requires written permission from Patryk Ankudowicz (Artisfera).
