/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 13:51:07 by hkanamit          #+#    #+#             */
/*   Updated: 2026/08/28 14:34:33 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

int main(int argc, char **argv)
{   
    int num_coders;
    int t_to_burnout;
    int t_to_compile;
    int t_to_debug;
    int t_to_refactor;
    int num_compile_req;
    int dongle_cooldown;
    char *scheduler;

    arguments ins;

    ins.num_coders = atoi(argv[1]);
    ins.t_to_burnout = atoi(argv[2]);
    ins.t_to_compile = atoi(argv[3]);
    ins.t_to_debug = atoi(argv[4]);
    ins.t_to_refactor = atoi(argv[5]);
    ins.num_compile_req = atoi(argv[6]);
    ins.dongle_cooldown = atoi(argv[7]);
    ins.scheduler = argv[8];
    
    run(ins);
    return(0);
}
