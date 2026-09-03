/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 13:51:07 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/02 18:33:15 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

bool is_valid_num(const char *s)
{
    int i = 0;
    while(s[i])
    {
        if (! s[0])
            return false;
        if(s[i] < '0'|| '9' < s[i])
            return false;
        i ++;
    }
    return true;
}
bool ori_atoi(const char *str, int *arg)
{
    if (is_valid_num(str) == 0)
        return false;
    int length = strlen(str) - 1;
    int num = 0;
    while(length)
    {
        num = num*10 + (str[length] - '0');
        length --;
    }
    if (num > INT_MAX)
        return false;
    *arg = num;
    return true;   
}
int is_valid_schedule(const char *s)
{
    if (!s[0])
        return 0;
    if (strcmp(*s, "fifo") == 0 || strcmp(*s, "edf") == 0)
        return true;
    else
        return false;
}


int main(int argc, char **argv)
{   
    if (argc != 9)
    {
        fprintf(stderr, "Error");
        return 1;
    }
    args ins;
    if (
        !(ori_atoi(argv[1], &ins.num_coders)&&
        ori_atoi(argv[2], &ins.t_to_burnout)&&
        ori_atoi(argv[3], &ins.t_to_compile)&&
        ori_atoi(argv[4], &ins.t_to_debug)&&
        ori_atoi(argv[5], &ins.t_to_refactor)&&
        ori_atoi(argv[6], &ins.num_compile_req)&&
        ori_atoi(argv[7], &ins.dongle_cooldown)
    ))
    {
        fprintf(stderr, "Error");
        return 1;
    }
    if (is_valid_schedule(argv[8]) == 0)
    {
        fprintf(stderr, "Error");
        return 1;
    }
    ins.scheduler = argv[8];

    
    return(0);
}
