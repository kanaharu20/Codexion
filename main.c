/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 13:51:07 by hkanamit          #+#    #+#             */
/*   Updated: 2026/08/31 14:54:33 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

int is_valid_num(const char *s)
{
    int i = 0;
    while(s[i])
    {
        if (! s[0])
            return 0;
        if(s[i] < '0'|| '9' < s[i])
            return 0;
        i ++;
    }
    if (i > 10)
        return 0;
    else
        return 1;
}
int is_valid_schedule(const char *s)
{
    if (!s[0])
        return 0;
    if (strcmp(*s, "fifo") == 0 || strcmp(*s, "edf") == 0)
        return 1;
    else
        return 0;
}
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

    args ins;

    if (argc != 9)//error
        return 1;
    
    int i = 1;
    while (i <= 7)
    {
        if (is_valid_num(argv[i]) == 0);
            return 1;//error
        i ++;
    }

    ins.num_coders = atoi(argv[1]);
    ins.t_to_burnout = atoi(argv[2]);
    ins.t_to_compile = atoi(argv[3]);
    ins.t_to_debug = atoi(argv[4]);
    ins.t_to_refactor = atoi(argv[5]);
    ins.num_compile_req = atoi(argv[6]);
    ins.dongle_cooldown = atoi(argv[7]);
    if (is_valid_schedule(argv[8]) == 0)
        return 1; //error
    ins.scheduler = argv[8];
    
    return(0);
}
