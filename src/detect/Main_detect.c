#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Args.h"
#include "Video.h"
#include "CCL.h"
#include "Features.h"
#include "Threshold.h"
#include "DebugUtil.h"
#include "macro_debug.h"
#include "KPPV.h"
#include "Tracking.h"
#include "Ballon.h"
#include "Validation.h"
#include "tools_visu.h"

#define SEQUENCE_DST_PATH_HIST "hist/"
#define SEQUENCE_NDIGIT 5

extern char path_stats_0[200];
extern char path_stats_1[200];
extern char path_frames_binary[250];
extern char path_frames_output[250];
extern char path_motion[200];
extern char path_extraction[200];
extern char path_error[200];
extern char path_tracks[200];
extern char path_bounding_box[200];
extern char path_debug[150];
extern char path_video_tracking[200];
extern uint32 *conflicts;
extern uint32 **nearest;
extern float32 **distances; 
extern elemBB *tabBB[NB_FRAMES];

// ======================================
void main_detect(int argc, char** argv)
// ======================================
{
    // default values
    int   def_start_frame    =      0;
    int   def_end_frame      = 200000;
    int   def_skip_frames    =      0;
    int   def_light_min      =     55;
    int   def_light_max      =     80;
    int   def_surface_min    =      3;
    int   def_surface_max    =   1000;
    int   def_k              =      3;
    int   def_r_extrapol     =      5;
    int   def_d_line         =     25;
    int   def_frame_star     =      3;
    float def_diff_deviation =    4.f;
    char* def_input_video    =   NULL;
    char* def_output_frames  =   NULL;
    char* def_output_bb      =   NULL;
    char* def_output_stats   =   NULL;

    // Help
    if (find_arg(argc, argv, "-h")) {
        fprintf(stderr, "  --input-video       Video source                                                         [%s]\n", def_input_video   );
        fprintf(stderr, "  --output-frames     Path frames output for debug                                         [%s]\n", def_output_frames );
        fprintf(stderr, "  --output-bb         Path to the file containing the bounding boxes (frame by frame)      [%s]\n", def_output_bb     );
        fprintf(stderr, "  --output-stats      TODO!                                                                [%s]\n", def_output_stats  );
        fprintf(stderr, "  --start-frame       Starting point of the video                                          [%d]\n", def_start_frame   );
        fprintf(stderr, "  --end-frame         Ending point of the video                                            [%d]\n", def_end_frame     );
        fprintf(stderr, "  --skip-frames       Number of skipped frames                                             [%d]\n", def_skip_frames   );
        fprintf(stderr, "  --light-min         Low hysteresis threshold (grayscale [0;255])                         [%d]\n", def_light_min     );
        fprintf(stderr, "  --light-max         High hysteresis threshold (grayscale [0;255])                        [%d]\n", def_light_max     );
        fprintf(stderr, "  --surface-min       Maximum area of the CC                                               [%d]\n", def_surface_min   );
        fprintf(stderr, "  --surface-max       Minimum area of the CC                                               [%d]\n", def_surface_max   );
        fprintf(stderr, "  -k                  Number of neighbours                                                 [%d]\n", def_k             );
        fprintf(stderr, "  --r-extrapol        Search radius for the next CC in case of extrapolation               [%d]\n", def_r_extrapol    );
        fprintf(stderr, "  --d-line            Position tolerance of a point going through a line                   [%d]\n", def_d_line        );
        fprintf(stderr, "  --frame-star        Minimum number of frames required to track a star                    [%d]\n", def_frame_star    );
        fprintf(stderr, "  --diff-deviation    Differential deviation factor for motion detection (motion error of      \n"                    );
        fprintf(stderr, "                      one CC has to be superior to diff_deviation * standard deviation)    [%f]\n", def_diff_deviation);
        fprintf(stderr, "  --track-all         Track all object types (star, meteor or noise)                           \n"                    );
        fprintf(stderr, "  -h                  This help                                                                \n"                    );
        exit(1);
    }

    // Parsing Arguments
    int start            = find_int_arg  (argc, argv, "--start-frame",    def_start_frame   );
    int end              = find_int_arg  (argc, argv, "--end-frame",      def_end_frame     );
    int skip             = find_int_arg  (argc, argv, "--skip-frames",    def_skip_frames   );
    int light_min        = find_int_arg  (argc, argv, "--light-min",      def_light_min     );
    int light_max        = find_int_arg  (argc, argv, "--light-max",      def_light_max     );
    int surface_min      = find_int_arg  (argc, argv, "--surface-min",    def_surface_min   );
    int surface_max      = find_int_arg  (argc, argv, "--surface-max",    def_surface_max   );
    int k                = find_int_arg  (argc, argv, "-k",               def_k             );
    int r_extrapol       = find_int_arg  (argc, argv, "--r-extrapol",     def_r_extrapol    );
    int d_line           = find_int_arg  (argc, argv, "--d-line",         def_d_line        );
    int frame_star       = find_int_arg  (argc, argv, "--frame-star",     def_frame_star    );
    float diff_deviation = find_float_arg(argc, argv, "--diff-deviation", def_diff_deviation);
    char* input_video    = find_char_arg (argc, argv, "--input-video",    def_input_video   );
    char* output_frames  = find_char_arg (argc, argv, "--output-frames",  def_output_frames );
    char* output_bb      = find_char_arg (argc, argv, "--output-bb",      def_output_bb     );
    char* output_stats   = find_char_arg (argc, argv, "--output-stats",   def_output_stats  );
    int   track_all      = find_arg      (argc, argv, "--track-all"                         );

    // heading display
    printf("#  -----------------------\n");
    printf("# |          ----*        |\n");
    printf("# | --* METEOR-DETECT --* |\n");
    printf("# |   -------*            |\n");
    printf("#  -----------------------\n");
    printf("#\n");
    printf("# Parameters:\n");
    printf("# -----------\n");
    printf("#  * input-video   = %s\n",    input_video);
    printf("#  * output-frames = %s\n",    output_frames);
    printf("#  * output-bb     = %s\n",    output_bb);
    printf("#  * output-stats  = %s\n",    output_stats);
    printf("#  * start-frame   = %d\n",    start);
    printf("#  * end-frame     = %d\n",    end);
    printf("#  * skip-frames   = %d\n",    skip);
    printf("#  * light-min     = %d\n",    light_min);
    printf("#  * light-max     = %d\n",    light_max);
    printf("#  * surface-min   = %d\n",    surface_min);
    printf("#  * surface-max   = %d\n",    surface_max);
    printf("#  * k             = %d\n",    k);
    printf("#  * r-extrapol    = %d\n",    r_extrapol);
    printf("#  * d-line        = %d\n",    d_line);
    printf("#  * frame-star    = %d\n",    frame_star);
    printf("#  * diff-deviaton = %4.2f\n", diff_deviation);
    printf("#  * track-all     = %d\n",    track_all);
    printf("#\n");

    if(!input_video){
        fprintf(stderr, "(EE) '--input-video' is missing\n"); exit(1);
    }
    if(!output_frames)
        fprintf(stderr, "(II) '--output-frames' is missing -> no frames will be saved\n");
    if(!output_stats)
        fprintf(stderr, "(II) '--output-stats' is missing -> no stats will be saved\n");

    // sequence
    char *filename;
	double theta, tx, ty;
    int frame;

    // CC
	MeteorROI stats0[SIZE_MAX_METEORROI];
	MeteorROI stats1[SIZE_MAX_METEORROI];
	MeteorROI stats_shrink[SIZE_MAX_METEORROI];
    Track tracks[SIZE_MAX_TRACKS];
    Track tracks_stars[SIZE_MAX_TRACKS];

    int  offset =  0;
    int  last   = -1;

	int n0 = 0;
	int n1 = 0;

    // image
    int b = 1;                  
    int i0, i1, j0, j1;

    //path management
    char *path;
    split_path_file(&path, &filename, input_video);
    disp(filename);
    if(output_stats) create_debug_dir (output_stats);
	if(output_frames) create_frames_dir(output_frames);
    if(output_bb) create_bb_file(output_bb);

    // ------------------------- //
    // -- INITIALISATION VIDEO-- //
    // ------------------------- //
    PUTS("INIT VIDEO");
    Video* video = Video_init_from_file(input_video, start, end, skip, &i0, &i1, &j0, &j1);
    
    // ---------------- //
    // -- ALLOCATION -- //
    // ---------------- //
    PUTS("ALLOC");

    // struct for image processing
    Ballon* ballon = allocBallon(i0, i1, j0, j1, b);

    // -------------------------- //
    // -- INITIALISATION MATRIX-- //
    // -------------------------- //

    initBallon(ballon, i0, i1, j0, j1, b);
	kppv_init(0, SIZE_MAX_KPPV, 0, SIZE_MAX_KPPV);
	init_MeteorROI(stats0, SIZE_MAX_METEORROI);
	init_MeteorROI(stats1, SIZE_MAX_METEORROI);
    init_Track(tracks, SIZE_MAX_TRACKS);
    init_Track(tracks_stars, SIZE_MAX_TRACKS);
    CCL_LSL_init(i0, i1, j0, j1);
    initTabBB();

    disp(path_tracks);

    // ----------------//
    // -- TRAITEMENT --//
    // ----------------//

    PUTS("LOOP");
	if(!Video_nextFrame(video,ballon->I0)) { 
        exit(1);
    }

    printf("# The program is running...\n");
    unsigned n_frames = 0;
    unsigned n_tracks = 0, n_stars = 0, n_meteors = 0, n_noise = 0;
    while(Video_nextFrame(video,ballon->I1)) {
        
        frame = video->frame_current-2;

		fprintf(stderr, "(II) Frame n°%4d", frame);

		//---------------------------------------------------------//
        PUTS("\t Step 1 : seuillage low/high");
        copy_ui8matrix_ui8matrix(ballon->I0, i0, i1, j0, j1, ballon->SH); 
        copy_ui8matrix_ui8matrix(ballon->I0, i0, i1, j0, j1, ballon->SM);
		//---------------------------------------------------------//
        threshold_high(ballon->SM, i0, i1, j0, j1, light_min);
        threshold_high(ballon->SH, i0, i1, j0, j1, light_max);
     	//---------------------------------------------------------//
        convert_ui8matrix_ui32matrix(ballon->SM, i0, i1, j0, j1, ballon->SM32); 
        convert_ui8matrix_ui32matrix(ballon->SH, i0, i1, j0, j1, ballon->SH32);

     	//--------------------------------------------------------//
        PUTS("\t Step 2 : ECC/ACC");
        n1 = CCL_LSL(ballon->SM32, i0, i1, j0, j1);
        idisp(n1);
        extract_features(ballon->SM32, i0, i1, j0, j1, stats1, n1);

     	//--------------------------------------------------------//
        PUTS("\t Step 3 : seuillage hysteresis && filter surface"); 
        merge_HI_CCL_v2(ballon->SH32, ballon->SM32, i0, i1, j0, j1, stats1, n1, surface_min, surface_max); 
        int n_shrink = shrink_stats(stats1, stats_shrink, n1);

      	//--------------------------------------------------------//
        PUTS("\t Step 4 : mise en correspondance");
		kppv_routine(stats0, stats_shrink, n0, n_shrink, k);

      	//--------------------------------------------------------//
        PUTS("\t Step 5 : recalage");
        motion(stats0, stats_shrink, n0, n_shrink, &theta, &tx, &ty);

      	//--------------------------------------------------------//
        PUTS("\t Step 6: Tracking");
        Tracking(stats0, stats_shrink, tracks, n0, n_shrink, frame, &last, &offset, theta, tx, ty, r_extrapol, d_line, diff_deviation, track_all, frame_star);
        
        //--------------------------------------------------------//
        PUTS("\t [DEBUG] Saving frames");
        if (output_frames){
	        create_frames_files(frame);
            disp(path_frames_binary);
            saveFrame_ui32matrix(path_frames_binary, ballon->SH32, i0, i1, j0, j1);
            // saveFrame_ui8matrix(path_frames_binary, ballon->I0, i0, i1, j0, j1);
        }

        PUTS("\t [DEBUG] Saving stats");
        if (output_stats){
    	    //create_debug_files (frame);
            disp(path_debug);
            saveAssoConflicts(path_debug, frame, conflicts, nearest, distances, n0, n_shrink, stats0, stats_shrink, tracks, last+1);
            // saveMotion(path_motion, theta, tx, ty, frame-1);
            // saveMotionExtraction(path_extraction, stats0, stats_shrink, n0, theta, tx, ty, frame-1);
            // saveError(path_error, stats0, n0);
        }

      	//--------------------------------------------------------//
        SWAP_UI8(ballon->I0, ballon->I1);
        SWAP_STATS(stats0, stats_shrink, n_shrink);
        n0 = n_shrink;
        n_frames++;

        n_tracks = track_count_objects(tracks, (unsigned)last+1, &n_stars, &n_meteors, &n_noise);
        fprintf(stderr, " -- Tracks = ['meteor': %3d, 'star': %3d, 'noise': %3d, 'total': %3d]\r", n_meteors, n_stars, n_noise, n_tracks);
        fflush(stderr);
    }
    fprintf(stderr, "\n");
    
    if (output_bb)
        saveTabBB(path_bounding_box, tabBB, tracks, NB_FRAMES, track_all);
    //saveTracks(path_tracks, tracks, last);
    printTracks2(stdout, tracks, last+1);

    printf("# Statistics:\n");
    printf("# -> Processed frames = %4d\n", n_frames);
    printf("# -> Detected tracks = ['meteor': %3d, 'star': %3d, 'noise': %3d, 'total': %3d]\n", n_meteors, n_stars, n_noise, n_tracks);

    // ----------
    // -- free --
    // ----------

    freeBallon(ballon, i0, i1, j0, j1, b);

    free(path);
    free(filename);
    Video_free(video);
    CCL_LSL_free(i0, i1, j0, j1);
	kppv_free(0, 50, 0, 50);
    printf("# End of the program, exiting.\n");
}

int main(int argc, char** argv)
{
    init_global_data();
    main_detect(argc, argv);
    return 0;
}
