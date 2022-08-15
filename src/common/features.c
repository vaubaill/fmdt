/*
 * Copyright (c) 2017-2018, Arthur Hennequin, LIP6, UPMC, CNRS
 * Copyright (c) 2020-2020, Lionel Lacassagne, all rights reserved, LIP6 Sorbonne University, CNRS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tools.h"
#include "features.h"

void features_init_ROI(ROI_t* stats, int n) {
    for (int i = 0; i < n; i++)
        memset(stats + i, 0, sizeof(ROI_t));
}

void features_extract(uint32_t** img, int i0, int i1, int j0, int j1, ROI_t* stats, int n) {
    for (int i = 1; i <= n; i++) {
        memset(stats + i, 0, sizeof(ROI_t));
        stats[i].xmin = j1;
        stats[i].xmax = j0;
        stats[i].ymin = i1;
        stats[i].ymax = i0;
    }

    for (int i = i0; i <= i1; i++) {
        for (int j = j0; j <= j1; j++) {
            uint32_t e = img[i][j];
            if (e > 0) {
                stats[e].S += 1;
                stats[e].ID = e;
                stats[e].Sx += j;
                stats[e].Sy += i;
                stats[e].Sx2 += j * j;
                stats[e].Sy2 += i * i;
                stats[e].Sxy += j * i;
                if (j < stats[e].xmin)
                    stats[e].xmin = j;
                if (j > stats[e].xmax)
                    stats[e].xmax = j;
                if (i < stats[e].ymin)
                    stats[e].ymin = i;
                if (i > stats[e].ymax)
                    stats[e].ymax = i;
            }
        }
    }

    for (int i = 1; i <= n; i++) {
        stats[i].x = (double)stats[i].Sx / (double)stats[i].S;
        stats[i].y = (double)stats[i].Sy / (double)stats[i].S;
    }
}

void features_merge_HI_CCL_v2(uint32_t** HI, uint32_t** M, int i0, int i1, int j0, int j1, ROI_t* stats, int n, int S_min,
                              int S_max) {
    int x0, x1, y0, y1, id;
    ROI_t cc;

    for (int i = 1; i <= n; i++) {
        cc = stats[i];
        if (cc.S) {
            id = cc.ID;
            if (S_min > cc.S || cc.S > S_max) {
                stats[i].S = 0;
                /* JUSTE POUR DEBUG (Affichage frames)*/
                x0 = cc.ymin;
                x1 = cc.ymax;
                y0 = cc.xmin;
                y1 = cc.xmax;
                for (int k = x0; k <= x1; k++) {
                    for (int l = y0; l <= y1; l++) {
                        if (M[k][l] == id)
                            HI[k][l] = 0;
                    }
                }
                continue;
            }
            x0 = cc.ymin;
            x1 = cc.ymax;
            y0 = cc.xmin;
            y1 = cc.xmax;
            for (int k = x0; k <= x1; k++) {
                for (int l = y0; l <= y1; l++) {
                    if (HI[k][l]) {
                        for (k = x0; k < x1; k++) {
                            for (l = y0; l < y1; l++) {
                                if (M[k][l] == id)
                                    HI[k][l] = i;
                            }
                        }
                        goto next;
                    }
                }
            }
            stats[i].S = 0;
        next:;
        }
    }
}

void features_filter_surface(ROI_t* stats, int n, uint32_t** img, uint32_t threshold_min, uint32_t threshold_max) {
    // Doit on vraiment modifier l'image de départ? ou juste les stats.
    uint32_t S, e;
    int i0, i1, j0, j1;
    uint16_t id;

    for (int i = 1; i <= n; i++) {
        S = stats[i].S;
        id = stats[i].ID;

        if (S == 0)
            continue; // DEBUG

        if (S < threshold_min || S > threshold_max) {
            stats[i].S = 0;

            // pour affichage debbug
            i0 = stats[i].ymin;
            i1 = stats[i].ymax;
            j0 = stats[i].xmin;
            j1 = stats[i].xmax;
            for (int i = i0; i <= i1; i++) {
                for (int j = j0; j <= j1; j++) {
                    e = img[i][j];
                    if (e == id) {
                        img[i][j] = 0;
                    }
                }
            }
        }
    }
}

int features_shrink_stats(ROI_t* stats_src, ROI_t* stats_dest, int n) {
    int cpt = 0;
    for (int i = 1; i <= n; i++) {
        if (stats_src[i].S > 0) {
            cpt++;
            memcpy(stats_dest +cpt, stats_src +i, sizeof(ROI_t));
            stats_dest[cpt].ID = cpt;
        }
    }
    return cpt;
}

void features_rigid_registration(ROI_t* stats0, ROI_t* stats1, int n0, int n1, double* theta, double* tx, double* ty) {
    double Sx, Sxp, Sy, Syp, Sx_xp, Sxp_y, Sx_yp, Sy_yp;
    ROI_t cc0, cc1;
    double x0, y0, x1, y1;
    double a, b;
    double xg, yg, xpg, ypg;
    int asso;
    int cpt;

    Sx = 0;
    Sxp = 0;
    Sy = 0;
    Syp = 0;
    Sx_xp = 0;
    Sxp_y = 0;
    Sx_yp = 0;
    Sy_yp = 0;
    cpt = 0;

    // parcours tab assos
    for (int i = 1; i <= n0; i++) {
        cc0 = stats0[i];
        asso = stats0[i].next; // assos[i];

        if (cc0.S > 0 && asso) {
            cpt++;
            cc1 = stats1[stats0[i].next];

            Sx += cc0.x;
            Sy += cc0.y;
            Sxp += cc1.x;
            Syp += cc1.y;
        }
    }

    xg = Sx / cpt;
    yg = Sy / cpt;
    xpg = Sxp / cpt;
    ypg = Syp / cpt;

    Sx = 0;
    Sxp = 0;
    Sy = 0;
    Syp = 0;

    // parcours tab assos
    for (int i = 1; i <= n0; i++) {
        cc0 = stats0[i];
        asso = stats0[i].next;

        if (cc0.S > 0 && asso) {
            // cpt++;
            cc1 = stats1[stats0[i].next];

            x0 = cc0.x - xg;
            y0 = cc0.y - yg;
            x1 = cc1.x - xpg;
            y1 = cc1.y - ypg;

            Sx += x0;
            Sy += y0;
            Sxp += x1;
            Syp += y1;
            Sx_xp += x0 * x1;
            Sxp_y += x1 * y0;
            Sx_yp += x0 * y1;
            Sy_yp += y0 * y1;
        }
    }
    a = cpt * cpt * (Sx_yp - Sxp_y) + (1 - 2 * cpt) * (Sx * Syp - Sxp * Sy);
    b = cpt * cpt * (Sx_xp + Sy_yp) + (1 - 2 * cpt) * (Sx * Sxp + Syp * Sy);

    *theta = atan2(a, b);
    *tx = xpg - cos(*theta) * xg + sin(*theta) * yg;
    *ty = ypg - sin(*theta) * xg - cos(*theta) * yg;
}

void features_rigid_registration_corrected(ROI_t* stats0, ROI_t* stats1, int n0, int n1, double* theta, double* tx,
                                           double* ty, double errMoy, double eType) {
    double Sx, Sxp, Sy, Syp, Sx_xp, Sxp_y, Sx_yp, Sy_yp;
    ROI_t cc0, cc1;
    double x0, y0, x1, y1;
    double a, b;
    double xg, yg, xpg, ypg;
    int asso;
    int cpt;

    Sx = 0;
    Sxp = 0;
    Sy = 0;
    Syp = 0;
    Sx_xp = 0;
    Sxp_y = 0;
    Sx_yp = 0;
    Sy_yp = 0;
    cpt = 0;

    int cpt1 = 0;
    // parcours tab assos
    for (int i = 1; i <= n0; i++) {
        cc0 = stats0[i];

        if (fabs(stats0[i].error - errMoy) > eType) {
            stats0[i].motion = 1;
            cpt1++;
            continue;
        }
        asso = stats0[i].next; // assos[i];

        if (cc0.S > 0 && asso) {
            cpt++;
            cc1 = stats1[stats0[i].next];

            Sx += cc0.x;
            Sy += cc0.y;
            Sxp += cc1.x;
            Syp += cc1.y;
        }
    }

    xg = Sx / cpt;
    yg = Sy / cpt;
    xpg = Sxp / cpt;
    ypg = Syp / cpt;

    Sx = 0;
    Sxp = 0;
    Sy = 0;
    Syp = 0;

    // parcours tab assos
    for (int i = 1; i <= n0; i++) {
        cc0 = stats0[i];

        if (fabs(stats0[i].error - errMoy) > eType)
            continue;

        asso = stats0[i].next;

        if (cc0.S > 0 && asso) {
            // cpt++;
            cc1 = stats1[stats0[i].next];

            x0 = cc0.x - xg;
            y0 = cc0.y - yg;
            x1 = cc1.x - xpg;
            y1 = cc1.y - ypg;

            Sx += x0;
            Sy += y0;
            Sxp += x1;
            Syp += y1;
            Sx_xp += x0 * x1;
            Sxp_y += x1 * y0;
            Sx_yp += x0 * y1;
            Sy_yp += y0 * y1;
        }
    }
    a = cpt * cpt * (Sx_yp - Sxp_y) + (1 - 2 * cpt) * (Sx * Syp - Sxp * Sy);
    b = cpt * cpt * (Sx_xp + Sy_yp) + (1 - 2 * cpt) * (Sx * Sxp + Syp * Sy);

    *theta = atan2(a, b);
    *tx = xpg - cos(*theta) * xg + sin(*theta) * yg;
    *ty = ypg - sin(*theta) * xg - cos(*theta) * yg;
}

// TODO: Pour l'optimisation : faire une version errorMoy_corrected()
double features_error_moy(ROI_t* stats, int n) {
    double S = 0.0;
    int cpt = 0;

    for (int i = 1; i <= n; i++) {

        if (stats[i].motion || !stats[i].next)
            continue;

        S += stats[i].error;
        cpt++;
    }
    return S / cpt;
}

// TODO: Pour l'optimisation : faire une version ecartType_corrected()
double features_ecart_type(ROI_t* stats, int n, double errMoy) {
    double S = 0.0;
    int cpt = 0;
    float e;

    for (int i = 1; i <= n; i++) {

        if (stats[i].motion || !stats[i].next)
            continue;

        e = stats[i].error;
        S += ((e - errMoy) * (e - errMoy));
        cpt++;
    }
    return sqrt(S / cpt);
}

void features_motion_extraction(ROI_t* stats0, ROI_t* stats1, int nc0, double theta, double tx, double ty) {
    int cc1;
    double x, y, xp, yp;
    float dx, dy;
    float e;

    for (int i = 1; i <= nc0; i++) {
        cc1 = stats0[i].next; // assos[i];
        if (cc1) {
            // coordonees du point dans l'image I+1
            xp = stats1[cc1].x;
            yp = stats1[cc1].y;
            // calcul de (x,y) pour l'image I
            x = cos(theta) * (xp - tx) + sin(theta) * (yp - ty);
            y = cos(theta) * (yp - ty) - sin(theta) * (xp - tx);

            // pas besoin de stocker dx et dy (juste pour l'affichage du debug)
            dx = x - stats0[i].x;
            dy = y - stats0[i].y;
            stats0[i].dx = dx;
            stats0[i].dy = dy;

            e = sqrt(dx * dx + dy * dy);
            stats0[i].error = e;
        }
    }
}

void features_motion(ROI_t* stats0, ROI_t* stats1, int n0, int n1, double* theta, double* tx, double* ty) {
    features_rigid_registration(stats0, stats1, n0, n1, theta, tx, ty);
    features_motion_extraction(stats0, stats1, n0, *theta, *tx, *ty);

    double errMoy = features_error_moy(stats0, n0);
    double eType = features_ecart_type(stats0, n0, errMoy);

    // saveErrorMoy("first_error.txt", errMoy, eType);

    features_rigid_registration_corrected(stats0, stats1, n0, n1, theta, tx, ty, errMoy, eType);
    features_motion_extraction(stats0, stats1, n0, *theta, *tx, *ty);
}

int features_analyse_ellipse(ROI_t* stats, int n, float e_threshold) {
    float S, Sx, Sy, Sx2, Sy2, Sxy;
    float x_avg, y_avg, m20, m02, m11;
    float a2, b2, a, b, e;

    int true_n = 0;

    for (int i = 1; i <= n; i++) {
        S = stats[i].S;
        Sx = stats[i].Sx;
        Sy = stats[i].Sy;
        Sx2 = stats[i].Sx2;
        Sy2 = stats[i].Sy2;
        Sxy = stats[i].Sxy;

        if (S == 0)
            continue;

        x_avg = Sx / S;
        y_avg = Sy / S;

        // Moments centrés
        m20 = Sx2 / S - x_avg * x_avg; // var(x)
        m02 = Sy2 / S - y_avg * y_avg; // var(y)
        m11 = Sxy / S - x_avg * y_avg; // cov(x,y) ?

        a2 = (m20 + m02 + sqrt((m20 - m02) * (m20 - m02) + 4.0 * m11 * m11)) / (2.0 * S);
        b2 = (m20 + m02 - sqrt((m20 - m02) * (m20 - m02) + 4.0 * m11 * m11)) / (2.0 * S);

        a = sqrt(a2);
        b = sqrt(b2);

        e = a / b; // garder cette ligne une fois que tout sera corrige LL 2022
        // e = b / a; // FAUX car a grand rayon et b petit rayon par construction LL 2022
        // e = sqrt(a2 - b2) / a;
        // e = a / b;

        // float abs_diff = fabs(theta - stats[i].angle_UV);
        // float angle_diff = min(min(abs_diff, 360-abs_diff), fabs(180-abs_diff));

        // test inverse de ce qui est naturel de faire (coherent avec e = b/a, mais INVERSE !) LL 2020
        if (e > e_threshold /*&&  angle_diff > 10*/) {
            stats[i].S = 0;
            continue;
        }
        true_n++;
        /*#ifndef REAL_TIME_TEST
        printf("i = %2d | m20 = %8.1f | m02 = %8.1f | m11 = %8.1f | theta = %4.1f | a = %6.1f | b = %6.1f | e = %6.1f |
        |uv| = %6.1f | arg(uv) = %4.1f\n", i, m20, m02, m11, theta, a, b, e, stats[i].norm_UV, stats[i].angle_UV);
        #endif*/
    }
    return true_n;
}

void features_print_stats(ROI_t* stats, int n)
{
    int cpt = 0;
    for (int i = 1; i <= n; i++) {
        if (stats[i].S > 0) {
            cpt++;
        }
    }
    printf("Nombre de CC : %d\n", cpt);

    if (cpt == 0)
        return;

    for (int i = 1; i <= n; i++) {
        if (stats[i].S > 0)
            printf("%4d \t %4d \t %4d \t %4d \t %4d \t %3d \t %4d \t %4d \t %7.1f \t %7.1f \t %4d \t %4d \t %4d \t "
                   "%7.1lf \t %d\n",
                   stats[i].ID, stats[i].xmin, stats[i].xmax, stats[i].ymin, stats[i].ymax, stats[i].S, stats[i].Sx,
                   stats[i].Sy, stats[i].x, stats[i].y, stats[i].prev, stats[i].next, stats[i].time, stats[i].error,
                   stats[i].motion);
    }
    printf("\n");
}

void features_parse_stats(const char* filename, ROI_t* stats, int* n)
{
    char lines[200];
    int id, xmin, xmax, ymin, ymax, s, sx, sy, prev, next;
    double x, y;
    float dx, dy, error;
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "(EE) cannot open file '%s'\n", filename);
        return;
    }

    // pour l'instant, n représente l'id max des stas mais c'est à changer
    fgets(lines, 100, file);
    sscanf(lines, "%d", n);

    while (fgets(lines, 200, file)) {
        sscanf(lines, "%d %d %d %d %d %d %d %d %lf %lf %f %f %f %d %d", &id, &xmin, &xmax, &ymin, &ymax, &s, &sx, &sy,
               &x, &y, &dx, &dy, &error, &prev, &next);
        stats[id].ID = id;
        stats[id].xmin = xmin;
        stats[id].xmax = xmax;
        stats[id].ymin = ymin;
        stats[id].ymax = ymax;
        stats[id].S = s;
        stats[id].Sx = sx;
        stats[id].Sy = sy;
        stats[id].x = x;
        stats[id].y = y;
        stats[id].dx = dx;
        stats[id].dy = dy;
        stats[id].error = error;
        stats[id].prev = prev;
        stats[id].next = next;
    }
    fclose(file);
}

void features_save_stats_file(FILE* f, ROI_t* stats, int n, track_t* tracks) {
    int cpt = 0;
    for (int i = 1; i <= n; i++)
        if (stats[i].S != 0)
            cpt++;

    fprintf(f, "# Regions of interest (ROI) [%d]: \n", cpt);
    if (cpt) {
        fprintf(f, "# ------||----------------||---------------------------||---------------------------||-------------------\n");
        fprintf(f, "#   ROI ||      Track     ||        Bounding Box       ||   Surface (S in pixels)   ||      Center       \n");
        fprintf(f, "# ------||----------------||---------------------------||---------------------------||-------------------\n");
        fprintf(f, "# ------||------|---------||------|------|------|------||-----|----------|----------||---------|---------\n");
        fprintf(f, "#    ID ||   ID |    Type || xmin | xmax | ymin | ymax ||   S |       Sx |       Sy ||       x |       y \n");
        fprintf(f, "# ------||------|---------||------|------|------|------||-----|----------|----------||---------|---------\n");
    }

    for (int i = 1; i <= n; i++) {
        if (stats[i].S != 0) {
            fprintf(f, "   %4d || %4d | %s || %4d | %4d | %4d | %4d || %3d | %8d | %8d || %7.1f | %7.1f  \n",
                    stats[i].ID, stats[i].track_id,
                    g_obj_to_string_with_spaces[tracks[stats[i].track_id - 1].obj_type], stats[i].xmin,
                    stats[i].xmax, stats[i].ymin, stats[i].ymax, stats[i].S, stats[i].Sx, stats[i].Sy, stats[i].x,
                    stats[i].y);
        }
    }
}

void features_save_stats(const char* filename, ROI_t* stats, int n, track_t* tracks) {
    FILE* f = fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "(EE) error ouverture %s \n", filename);
        exit(1);
    }
    features_save_stats_file(f, stats, n, tracks);
    fclose(f);
}

void features_save_motion(const char* filename, double theta, double tx, double ty, int frame) {
    FILE* f = fopen(filename, "a");
    if (f == NULL) {
        fprintf(stderr, "(EE) error ouverture %s \n", filename);
        exit(1);
    }

    fprintf(f, "%d - %d\n", frame, frame + 1);
    fprintf(f, "%6.7f \t %6.4f \t %6.4f \n", theta, tx, ty);
    fprintf(f, "---------------------------------------------------------------\n");
    fclose(f);
}

void features_save_error(const char* filename, ROI_t* stats, int n) {
    double S = 0;
    int cpt = 0;
    FILE* f = fopen(filename, "a");
    if (f == NULL) {
        fprintf(stderr, "(EE) error ouverture %s \n", filename);
        exit(1);
    }

    for (int i = 1; i <= n; i++) {
        if (stats[i].S > 0) {
            S += stats[i].error;
            cpt++;
        }
    }

    fprintf(f, "%.2f\n", S / cpt);
    fclose(f);
}

void features_save_error_moy(const char* filename, double errMoy, double eType) {
    char path[200];
    FILE* f = fopen(filename, "a");
    if (f == NULL) {
        fprintf(stderr, "(EE) error ouverture %s \n", path);
        exit(1);
    }
    fprintf(f, "%5.2f \t %5.2f \n", errMoy, eType);
    fclose(f);
}

void features_save_motion_extraction(char* filename, ROI_t* stats0, ROI_t* stats1, int nc0, double theta, double tx,
                                     double ty, int frame) {
    // Version DEBUG : il faut implémenter une version pour le main
    FILE* f = fopen(filename, "a");
    if (f == NULL) {
        fprintf(stderr, "motion : cannot open file\n");
        return;
    }

    double errMoy = features_error_moy(stats0, nc0);
    double eType = features_ecart_type(stats0, nc0, errMoy);

    for (int i = 1; i <= nc0; i++) {
        float e = stats0[i].error;
        // si mouvement detecté
        if (fabs(e - errMoy) > 1.5 * eType) {
            fprintf(f, "%d - %d\n", frame, frame + 1);
            fprintf(f,
                    "CC en mouvement: %2d \t dx:%.3f \t dy: %.3f \t xmin: %3d \t xmax: %3d \t ymin: %3d \t ymax: %3d\n",
                    stats0[i].ID, stats0[i].dx, stats0[i].dy, stats0[i].xmin, stats0[i].xmax, stats0[i].ymin,
                    stats0[i].ymax);
            fprintf(f, "---------------------------------------------------------------\n");
        }
    }
    fclose(f);
}