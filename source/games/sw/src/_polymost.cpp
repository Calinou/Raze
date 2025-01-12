BEGIN_SW_NS

bool FindCeilingView(short match, int32_t* x, int32_t* y, int32_t z, int16_t* sectnum);
bool FindFloorView(short match, int32_t* x, int32_t* y, int32_t z, int16_t* sectnum);


short
ViewSectorInScene(short cursectnum, short level)
{
    int i;
    SPRITEp sp;
    short match;

    StatIterator it(STAT_FAF);
    while ((i = it.NextIndex()) >= 0)
    {
        sp = &sprite[i];

        if (sp->hitag == level)
        {
            if (cursectnum == sp->sectnum)
            {
                // ignore case if sprite is pointing up
                if (sp->ang == 1536)
                    continue;

                // only gets to here is sprite is pointing down

                // found a potential match
                match = sp->lotag;

                if (!testgotpic(FAF_MIRROR_PIC, true))
                    return -1;
                return match;
            }
        }
    }

    return -1;
}



void
DrawOverlapRoom(int tx, int ty, int tz, fixed_t tq16ang, fixed_t tq16horiz, short tsectnum)
{
    short i;
    short match;

    save.zcount = 0;

    match = ViewSectorInScene(tsectnum, VIEW_LEVEL1);
    if (match != -1)
    {
        FindCeilingView(match, &tx, &ty, tz, &tsectnum);

        if (tsectnum < 0)
            return;

        renderDrawRoomsQ16(tx, ty, tz, tq16ang, tq16horiz, tsectnum);

        // reset Z's
        for (i = 0; i < save.zcount; i++)
        {
            sector[save.sectnum[i]].floorz = save.zval[i];
            sector[save.sectnum[i]].floorpicnum = save.pic[i];
            sector[save.sectnum[i]].floorheinum = save.slope[i];
        }

        analyzesprites(pm_tsprite, pm_spritesortcnt, tx, ty, tz, false);
        post_analyzesprites(pm_tsprite, pm_spritesortcnt);
        renderDrawMasks();

    }
    else
    {
        match = ViewSectorInScene(tsectnum, VIEW_LEVEL2);
        if (match != -1)
        {
            FindFloorView(match, &tx, &ty, tz, &tsectnum);

            if (tsectnum < 0)
                return;

            renderDrawRoomsQ16(tx, ty, tz, tq16ang, tq16horiz, tsectnum);

            // reset Z's
            for (i = 0; i < save.zcount; i++)
            {
                sector[save.sectnum[i]].ceilingz = save.zval[i];
                sector[save.sectnum[i]].ceilingpicnum = save.pic[i];
                sector[save.sectnum[i]].ceilingheinum = save.slope[i];
            }

            analyzesprites(pm_tsprite, pm_spritesortcnt, tx, ty, tz, false);
            post_analyzesprites(pm_tsprite, pm_spritesortcnt);
            renderDrawMasks();

        }
    }
}

void FAF_DrawRooms(int x, int y, int z, fixed_t q16ang, fixed_t q16horiz, short sectnum)
{
    int i;
    StatIterator it(STAT_CEILING_FLOOR_PIC_OVERRIDE);
    while ((i = it.NextIndex()) >= 0)
    {
        if (SPRITE_TAG3(i) == 0)
        {
            // back up ceilingpicnum and ceilingstat
            SPRITE_TAG5(i) = sector[sprite[i].sectnum].ceilingpicnum;
            sector[sprite[i].sectnum].ceilingpicnum = SPRITE_TAG2(i);
            SPRITE_TAG4(i) = sector[sprite[i].sectnum].ceilingstat;
            //SET(sector[sprite[i].sectnum].ceilingstat, ((int)SPRITE_TAG7(i))<<7);
            SET(sector[sprite[i].sectnum].ceilingstat, SPRITE_TAG6(i));
            RESET(sector[sprite[i].sectnum].ceilingstat, CEILING_STAT_PLAX);
        }
        else if (SPRITE_TAG3(i) == 1)
        {
            SPRITE_TAG5(i) = sector[sprite[i].sectnum].floorpicnum;
            sector[sprite[i].sectnum].floorpicnum = SPRITE_TAG2(i);
            SPRITE_TAG4(i) = sector[sprite[i].sectnum].floorstat;
            //SET(sector[sprite[i].sectnum].floorstat, ((int)SPRITE_TAG7(i))<<7);
            SET(sector[sprite[i].sectnum].floorstat, SPRITE_TAG6(i));
            RESET(sector[sprite[i].sectnum].floorstat, FLOOR_STAT_PLAX);
        }
    }

    renderDrawRoomsQ16(x,y,z,q16ang,q16horiz,sectnum);

    it.Reset(STAT_CEILING_FLOOR_PIC_OVERRIDE);
    while ((i = it.NextIndex()) >= 0)
    {
        // manually set gotpic
        if (gotsector[sprite[i].sectnum])
        {
            setgotpic(FAF_MIRROR_PIC);
        }

        if (SPRITE_TAG3(i) == 0)
        {
            // restore ceilingpicnum and ceilingstat
            sector[sprite[i].sectnum].ceilingpicnum = SPRITE_TAG5(i);
            sector[sprite[i].sectnum].ceilingstat = SPRITE_TAG4(i);
            //RESET(sector[sprite[i].sectnum].ceilingstat, CEILING_STAT_TYPE_MASK);
            RESET(sector[sprite[i].sectnum].ceilingstat, CEILING_STAT_PLAX);
        }
        else if (SPRITE_TAG3(i) == 1)
        {
            sector[sprite[i].sectnum].floorpicnum = SPRITE_TAG5(i);
            sector[sprite[i].sectnum].floorstat = SPRITE_TAG4(i);
            //RESET(sector[sprite[i].sectnum].floorstat, FLOOR_STAT_TYPE_MASK);
            RESET(sector[sprite[i].sectnum].floorstat, FLOOR_STAT_PLAX);
        }
    }
}

void polymost_drawscreen(PLAYERp pp, int tx, int ty, int tz, binangle tang, fixedhoriz thoriz, int tsectnum)
{
    videoSetCorrectedAspect();
    renderSetAspect(xs_CRoundToInt(double(viewingrange) * tan(r_fov * (pi::pi() / 360.))), yxaspect);
    OverlapDraw = true;
    DrawOverlapRoom(tx, ty, tz, tang.asq16(), thoriz.asq16(), tsectnum);
    OverlapDraw = false;

    if (automapMode != am_full)// && !ScreenSavePic)
    {
        // TEST this! Changed to camerapp
        //JS_DrawMirrors(camerapp, tx, ty, tz, tang.asq16(), thoriz.asq16());
        JS_DrawMirrors(pp, tx, ty, tz, tang.asq16(), thoriz.asq16());
    }

    // TODO: This call is redundant if the tiled overhead map is shown, but the
    // HUD elements should be properly outputted with hardware rendering first.
    if (!FAF_DebugView)
        FAF_DrawRooms(tx, ty, tz, tang.asq16(), thoriz.asq16(), tsectnum);

    analyzesprites(pm_tsprite, pm_spritesortcnt, tx, ty, tz, tang.asbuild());
    post_analyzesprites(pm_tsprite, pm_spritesortcnt);
    renderDrawMasks();

}

void JS_DrawMirrors(PLAYERp pp, int tx, int ty, int tz,  fixed_t tpq16ang, fixed_t tpq16horiz)
{
    int j, cnt;
    int dist;
    int tposx, tposy; // Camera
    int *longptr;
    fixed_t tang;

//    int tx, ty, tz, tpang;             // Interpolate so mirror doesn't
    // drift!
    bool bIsWallMirror = false;

    {
        for (cnt = MAXMIRRORS - 1; cnt >= 0; cnt--)
            //if (testgotpic(cnt + MIRRORLABEL) || testgotpic(cnt + CAMSPRITE))
            if (testgotpic(cnt + MIRRORLABEL) || ((unsigned)mirror[cnt].campic < MAXTILES && testgotpic(mirror[cnt].campic)))
            {
                bIsWallMirror = false;
                if (testgotpic(cnt + MIRRORLABEL, true))
                {
                    bIsWallMirror = true;
                }
                else if ((unsigned)mirror[cnt].campic < MAXTILES && testgotpic(mirror[cnt].campic))
                {
                    cleargotpic(mirror[cnt].campic);
                }

                mirrorinview = true;

//                tx = interpolatedvalue(pp->oposx, pp->posx, smoothratio);
//                ty = interpolatedvalue(pp->oposy, pp->posy, smoothratio);
//                tz = interpolatedvalue(pp->oposz, pp->posz, smoothratio);
//                tpq16ang = pp->angle.ang.asq16();


                dist = 0x7fffffff;

                if (bIsWallMirror)
                {
                    j = abs(wall[mirror[cnt].mirrorwall].x - tx);
                    j += abs(wall[mirror[cnt].mirrorwall].y - ty);
                    if (j < dist)
                        dist = j;
                }
                else
                {
                    SPRITEp tp;

                    tp = &sprite[mirror[cnt].camsprite];

                    j = abs(tp->x - tx);
                    j += abs(tp->y - ty);
                    if (j < dist)
                        dist = j;
                }

                if (mirror[cnt].ismagic)
                {
                    SPRITEp sp=NULL;
                    int camhoriz;
                    short w;
                    int dx, dy, dz, tdx, tdy, tdz, midx, midy;


                    ASSERT(mirror[cnt].camera != -1);

                    sp = &sprite[mirror[cnt].camera];

                    ASSERT(sp);

                    // Calculate the angle of the mirror wall
                    w = mirror[cnt].mirrorwall;

                    // Get wall midpoint for offset in mirror view
                    midx = (wall[w].x + wall[wall[w].point2].x) / 2;
                    midy = (wall[w].y + wall[wall[w].point2].y) / 2;

                    // Finish finding offsets
                    tdx = abs(midx - tx);
                    tdy = abs(midy - ty);

                    if (midx >= tx)
                        dx = sp->x - tdx;
                    else
                        dx = sp->x + tdx;

                    if (midy >= ty)
                        dy = sp->y - tdy;
                    else
                        dy = sp->y + tdy;

                    tdz = abs(tz - sp->z);
                    if (tz >= sp->z)
                        dz = sp->z + tdz;
                    else
                        dz = sp->z - tdz;


                    // Is it a TV cam or a teleporter that shows destination?
                    // true = It's a TV cam
                    mirror[cnt].mstate = m_normal;
                    if (TEST_BOOL1(sp))
                        mirror[cnt].mstate = m_viewon;

                    // Show teleport destination
                    // NOTE: Adding MAXSECTORS lets you draw a room, even if
                    // you are outside of it!
                    if (mirror[cnt].mstate != m_viewon)
                    {
						tileDelete(MIRROR);
                        // Set TV camera sprite size to 0 to show mirror
                        // behind in this case!

                        if (mirror[cnt].campic != -1)
							tileDelete(mirror[cnt].campic);
                        renderDrawRoomsQ16(dx, dy, dz, tpq16ang, tpq16horiz, sp->sectnum + MAXSECTORS);
                        analyzesprites(pm_tsprite, pm_spritesortcnt, dx, dy, dz, false);
                        renderDrawMasks();
                    }
                }
                else
                {
                    // It's just a mirror
                    // Prepare drawrooms for drawing mirror and calculate
                    // reflected
                    // position into tposx, tposy, and tang (tposz == cposz)
                    // Must call preparemirror before drawrooms and
                    // completemirror after drawrooms
					display_mirror = true;
                    renderPrepareMirror(tx, ty, tz, tpq16ang, tpq16horiz,
                                  mirror[cnt].mirrorwall, /*mirror[cnt].mirrorsector,*/ &tposx, &tposy, &tang);

                    renderDrawRoomsQ16(tposx, tposy, tz, (tang), tpq16horiz, mirror[cnt].mirrorsector + MAXSECTORS);

                    analyzesprites(pm_tsprite, pm_spritesortcnt, tposx, tposy, tz, tang >> 16);
                    renderDrawMasks();

                    renderCompleteMirror();   // Reverse screen x-wise in this
					display_mirror = false;
                }


                // g_visibility = tvisibility;
                // g_visibility = NormalVisibility;

                // renderDrawRoomsQ16(tx, ty, tz, tpq16ang, tpq16horiz, pp->cursectnum);
                // Clean up anything that the camera view might have done
				tileDelete(MIRROR);
                wall[mirror[cnt].mirrorwall].overpicnum = MIRRORLABEL + cnt;
            }
            else
                mirrorinview = false;
    }
}


END_SW_NS
