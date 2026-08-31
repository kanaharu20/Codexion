/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   header.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 13:48:41 by hkanamit       #+#    #+#             */
/*   Updated: 2026/08/28 13:50:51 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HEADER_H
#define HEADER_H
#include <stdlib.h>
#include <string.h>
typedef struct {
    int num_coders;
    int t_to_burnout;
    int t_to_compile;
    int t_to_debug;
    int t_to_refactor;
    int num_compile_req;
    int dongle_cooldown;
    char *scheduler;
} args;


#endif