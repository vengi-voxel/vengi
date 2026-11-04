// SDL_GetPrefPath() stores user files below /libsdl. Restore them before main
// reads settings, and persist writes while the application is running.
Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
    addRunDependency('vengi-filesystem');
    try {
        FS.mkdirTree('/libsdl');
        FS.mount(IDBFS, {autoPersist: true}, '/libsdl');
        FS.syncfs(true, function (err) {
            if (err) {
                console.error('Filesystem restore error:', err);
            }
            removeRunDependency('vengi-filesystem');
        });
    } catch (err) {
        console.error('Filesystem initialization error:', err);
        removeRunDependency('vengi-filesystem');
    }
});
