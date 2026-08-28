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

typedef struct {
    int num_coders;
    float t_to_burnout;
    float t_to_compile;
    float t_to_debug;
    float t_to_refactor;
    float num_compile_req;
    float dongle_cooldown;
    char *scheduler;
} arguments;

#endif