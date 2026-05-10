# firmware/data/

Drop the bundled Watcher Web Console here as `index.html`.

The recommended way to produce it:

1. From the project root, generate the standalone bundle:
   `super_inline_html` → `Watcher Web Console (standalone).html`
2. Copy that file to `firmware/data/index.html`.
3. The firmware's `EMBED_FILES` directive in `main/CMakeLists.txt` bakes it
   into the app binary; no separate flash partition needed.
4. On `GET /`, the firmware finds the `<script id="watcher-bootstrap">` block
   and rewrites its contents with the live NVS state before sending.

Empty-data flavour (fresh device, no demo content): copy
`Watcher Web Console (esp32-empty).html` → `firmware/data/index.html`.
