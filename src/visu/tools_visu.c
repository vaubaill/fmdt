/*
 * Copyright (c) 2022, Clara Ciocan/ Mathuran Kandeepan
 * LIP6
 */

#include <stdio.h>
#include <string.h>
#ifdef OPENCV_LINK
#include <tuple>
#include <vector>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#endif

#include "debug_utils.h"
#include "tools_visu.h"

rgb8 tools_get_color(enum color_e color) {
    rgb8 gray;
    gray.g = 175;
    gray.b = 175;
    gray.r = 175;

    rgb8 green;
    green.g = 255;
    green.b = 000;
    green.r = 000;

    rgb8 red;
    red.g = 000;
    red.b = 000;
    red.r = 255;

    rgb8 blue;
    blue.g = 000;
    blue.b = 255;
    blue.r = 000;

    rgb8 purple;
    purple.g = 127;
    purple.b = 255;
    purple.r = 127;

    rgb8 orange;
    orange.r = 255;
    orange.g = 165;
    orange.b = 000;

    rgb8 yellow;
    yellow.g = 255;
    yellow.b = 000;
    yellow.r = 255;

    rgb8 misc;
    misc.g = 255;
    misc.b = 153;
    misc.r = 153;

    switch (color) {
    case GRAY:
        return gray;
    case GREEN:
        return green;
    case RED:
        return red;
    case BLUE:
        return blue;
    case PURPLE:
        return purple;
    case ORANGE:
        return orange;
    case YELLOW:
        return yellow;
    case MISC:
        return misc;
    default:
        break;
    }
    return red;
}

#ifdef OPENCV_LINK // this is C++ code (because OpenCV API is C++ now)
void tools_draw_legend_squares(rgb8** img, unsigned box_size, unsigned h_space, unsigned v_space, int validation) {
    //                     ymin      ymax      xmin      xmax     color
    std::vector<std::tuple<unsigned, unsigned, unsigned, unsigned, rgb8>> box_list;

    for (int i = 0; i < N_OBJECTS; i++)
        box_list.push_back(std::make_tuple((i + 1) * v_space + (i + 0) * box_size,    // ymin
                                           (i + 1) * v_space + (i + 1) * box_size,    // ymax
                                           (+1) * h_space + (+0) * box_size,          // xmin
                                           (+1) * h_space + (+1) * box_size,          // xmax
                                           tools_get_color(g_obj_type_to_color[i]))); // color

    if (validation)
        // add false positve meteor
        box_list.push_back(std::make_tuple((N_OBJECTS + 2) * v_space + (N_OBJECTS + 1) * box_size, // ymin
                                           (N_OBJECTS + 2) * v_space + (N_OBJECTS + 2) * box_size, // ymax
                                           (+1) * h_space + (+0) * box_size,                       // xmin
                                           (+1) * h_space + (+1) * box_size,                       // xmax
                                           tools_get_color(RED)));                                 // color

    for (auto& box : box_list)
        plot_bounding_box(img, std::get<0>(box), std::get<1>(box), std::get<2>(box), std::get<3>(box), 2,
                          std::get<4>(box));
}

void tools_draw_legend_texts(cv::Mat& cv_img, unsigned box_size, unsigned h_space, unsigned v_space, int validation) {
    //                          color        pos         text
    std::vector<std::tuple<cv::Scalar, cv::Point, std::string>> txt_list;
    for (int i = 0; i < N_OBJECTS; i++) {
        rgb8 color = tools_get_color(g_obj_type_to_color[i]);
        unsigned x = 2 * h_space + box_size;
        unsigned y = ((i + 1) * v_space + (i + 1) * box_size) - 2;
        txt_list.push_back(std::make_tuple(cv::Scalar(color.b, color.g, color.r), cv::Point(x, y),
                                           std::string(g_obj_type_to_string[i])));
    }

    if (validation) {
        // add false positve meteor
        rgb8 color = tools_get_color(RED);
        unsigned x = 2 * h_space + box_size;
        unsigned y = ((N_OBJECTS + 2) * v_space + (N_OBJECTS + 2) * box_size) - 2;
        txt_list.push_back(
            std::make_tuple(cv::Scalar(color.b, color.g, color.r), cv::Point(x, y), std::string("fp meteor")));
    }

    for (auto& txt : txt_list)
        cv::putText(cv_img,
                    std::get<2>(txt).c_str(), // text
                    std::get<1>(txt),         // position
                    cv::FONT_HERSHEY_DUPLEX,  // font type
                    0.7,                      // font size
                    std::get<0>(txt),         // color
                    1,                        // ?
                    cv::LINE_AA);             // ?
}

void tools_draw_track_ids(cv::Mat& cv_img, const BB_coord_t* listBB, const int nBB) {
    //                       x    y color        list of ids
    std::vector<std::tuple<int, int, rgb8, std::vector<int>>> list_of_ids_grouped_by_pos;
    for (int i = 0; i < nBB; i++) {
        int x = listBB[i].xmax + 3;
        int y = (listBB[i].ymin) + ((listBB[i].ymax - listBB[i].ymin) / 2);

        bool found = false;
        for (auto& l : list_of_ids_grouped_by_pos) {
            rgb8 c = tools_get_color(listBB[i].color);
            if (std::get<0>(l) == x && std::get<1>(l) == y && std::get<2>(l).r == c.r && std::get<2>(l).g == c.g &&
                std::get<2>(l).b == c.b) {
                std::get<3>(l).push_back(listBB[i].track_id);
                found = true;
            }
        }

        if (!found) {
            std::vector<int> v;
            v.push_back(listBB[i].track_id);
            list_of_ids_grouped_by_pos.push_back(std::make_tuple(x, y, tools_get_color(listBB[i].color), v));
        }
    }

    for (auto id : list_of_ids_grouped_by_pos) {
        std::string txt = std::to_string(std::get<3>(id)[std::get<3>(id).size() - 1]);
        for (int s = std::get<3>(id).size() - 2; s >= 0; s--)
            txt += "," + std::to_string(std::get<3>(id)[s]);

        const int x = std::get<0>(id);
        const int y = std::get<1>(id);
        const rgb8 color = std::get<2>(id);

        // writing 'txt' over the image
        cv::Point org(x, y);
        cv::putText(cv_img, txt.c_str(), org, cv::FONT_HERSHEY_DUPLEX, 0.7, cv::Scalar(color.b, color.g, color.r), 1,
                    cv::LINE_AA);
    }
}

void tools_draw_text(rgb8** img, const int img_width, const int img_height, const BB_coord_t* listBB, const int nBB,
                     int validation, int show_ids) {
    unsigned box_size = 20, h_space = 10, v_space = 10;
    tools_draw_legend_squares(img, box_size, h_space, v_space, validation);

    // create a blank image of size
    // (img_width x img_height) with white background
    // (B, G, R) : (255, 255, 255)
    cv::Mat cv_img(img_height, img_width, CV_8UC3, cv::Scalar(255, 255, 255));

    // check if the image is created successfully
    if (!cv_img.data) {
        std::cerr << "(EE) Could not open or find the image" << std::endl;
        std::exit(-1);
    }

    // convert: 'img' into 'cv::Mat'
    for (int i = 0; i < img_height; i++)
        for (int j = 0; j < img_width; j++) {
            cv_img.at<cv::Vec3b>(i, j)[2] = img[i][j].r;
            cv_img.at<cv::Vec3b>(i, j)[1] = img[i][j].g;
            cv_img.at<cv::Vec3b>(i, j)[0] = img[i][j].b;
        }

    if (show_ids)
        tools_draw_track_ids(cv_img, listBB, nBB);
    tools_draw_legend_texts(cv_img, box_size, h_space, v_space, validation);

    // // debug: show image inside a window.
    // cv::imshow("Output", cv_img);
    // cv::waitKey(0);

    // convert back: 'cv::Mat' into 'img'
    for (int i = 0; i < img_height; i++) {
        for (int j = 0; j < img_width; j++) {
            img[i][j].r = cv_img.at<cv::Vec3b>(i, j)[2];
            img[i][j].g = cv_img.at<cv::Vec3b>(i, j)[1];
            img[i][j].b = cv_img.at<cv::Vec3b>(i, j)[0];
        }
    }
}
#endif

void tools_convert_img_grayscale_to_rgb(const uint8** I, rgb8** I_bb, int i0, int i1, int j0, int j1) {
    for (int i = i0; i <= i1; i++) {
        for (int j = j0; j <= j1; j++) {
            I_bb[i][j].r = I[i][j];
            I_bb[i][j].g = I[i][j];
            I_bb[i][j].b = I[i][j];
        }
    }
}

void tools_draw_BB(rgb8** I_bb, const BB_coord_t* listBB, int n_BB) {
    for (int i = 0; i < n_BB; i++)
        plot_bounding_box(I_bb, listBB[i].ymin, listBB[i].ymax, listBB[i].xmin, listBB[i].xmax, 2,
                          tools_get_color(listBB[i].color));
}

void tools_save_frame(const char* filename, const rgb8** I_bb, int w, int h) {
    char buffer[80];

    FILE* file;
    file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "(EE) Failed opening '%s' file\n", filename);
        exit(-1);
    }

    /* enregistrement de l'image au format rpgm */
    sprintf(buffer, "P6\n%d %d\n255\n", (int)(w - 1), (int)(h - 1));
    fwrite(buffer, strlen(buffer), 1, file);
    for (int i = 0; i < h; i++)
        write_PNM_row((uint8*)I_bb[i], w - 1, file);

    /* fermeture du fichier */
    fclose(file);
}
