    - added a fallback generic record to grpinfo.txt for identifying Blood.rff based on content.
    - gave key 7 a proper spawn record using the blue outline as image. A proper definition here is needed to allow dropping this item. The original code had a picnum of -1 here which caused crashes.
    - cleanup of movie player code, migration to event interface.
    - Screen Job refactoring to fix the volatile timer in there causing problems with the menu.
    - split out the movie player into its own file.
    - Duke: Clamp RRRA vehicle input in `processVehicleInput()`.
    - Duke: Add `resurrected` flag to handle resurrection via cheating or when pissing in RR.
    - added a filter to the directory loader to remove EDuke32's texture cache files. These cause problems with the texture manager.
    - make map art work.
    - allow specifying startup .con files via GAMEINFO.
    - used the newly added game ID as reference for GAMEINFO to autoselect which game to start a mod with.
    - added GameID field to GrpInfo. This is for allowing new features easier referencing of the various records.
    - Exhumed: Redo player panning code, but guard it with cl_slopetilting as it does not work that well with mouselook.
    - Exhumed: fix for moving on sloped floors
    - added widescreen graphics credits to the Engine Credits menu.
    - Blood: default skill is 3, not 2.
    - fixed some bogus range checks in automap code.
    - fixed the vertical offsets of the World Tour skies. They were rendered too low.
    - added native support for Nightfright's Alien World Order" GRP generator.
    - enable embedding of blood.rff and sounds.rff in mod archives when playing Blood Some mods provide pregenerated resources, this allows loading them without picking them apart first.
    - Blood: add a dummy sound entry at index 0 so that no valid sound gets placed in this slot.
    - Blood: fixed issue with INI detection when having content added by RFS files.
    - added PlaySound CCNDs.
    - Blood: fixed mixup of values 0 and -1 in sound code.
    - added CHANF_FORCE flag for forcing non-looped sounds to start, even when sound is paused.
    - make sure voxels are being precached.
    - disabled the QAV preload calls in Blood. This is ultimately more harmful than useful as it forces loading of a large number of textures at the same time during gameplay instead of spreading them out.
    - fixed texture precaching. After the migration to GZDoom's full backend this never created any textures when precaching things.
    - fixed: alpha was never set for voxels.
    - fixed palette setup for duplicate base palettes. Fixes #301 - Blood's invulnerability palette is identical to the base.
    - activate the progress bar on the startup screen.
    - make the startup banner in the initial console window work.
    - Blood: undid restriction for original QAV for Guns Akimbo shotgun fix.
