Build
```sh
flatpak-builder --force-clean --user --install-deps-from=flathub --repo=repo --install builddir org.neocities.twinklepillar.swaap.json
```

Bundle
```sh
flatpak build-bundle repo org.neocities.twinklepillar.swaap.flatpak org.neocities.twinklepillar.swaap --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
```
