#pragma OPENCL EXTENSION cl_khr_fp16 : enable
__constant sampler_t smp_zero = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
__kernel void conv2d_transpose2x2_NHWC4(__read_only image2d_t src_data, __global FLT16 *weight,
                                        __read_only image2d_t biases, __write_only image2d_t dst_data, int2 kernel_size,
                                        int2 stride, int2 padding, int4 src_size, int4 dst_size) {
  int h = get_global_id(0);
  int kh = h % 2;
  int src_h = h / 2;
  src_h = src_h * 2;
  int w = get_global_id(1);
  int kw = w % 2;
  int src_w = w / 2;
  src_w = src_w * 2;
  int co = get_global_id(2);
  if (src_h * 2 >= dst_size.x || src_w * 2 >= dst_size.y || co >= dst_size.z) return;
  FLT4 r0 = (FLT4)(0.f);
  FLT4 r1 = (FLT4)(0.f);
  FLT4 r2 = (FLT4)(0.f);
  FLT4 r3 = (FLT4)(0.f);
  int base_w = (co * 4 + kh * 2 + kw) * src_size.z;
  for (int ci = 0; ci < src_size.z; ++ci) {
    FLT4 x0 = READ_IMAGE(src_data, smp_zero, (int2)(src_w * src_size.z + ci, src_h));
    FLT4 x1 = READ_IMAGE(src_data, smp_zero, (int2)(src_w * src_size.z + ci, src_h + 1));
    FLT4 x2 = READ_IMAGE(src_data, smp_zero, (int2)((src_w + 1) * src_size.z + ci, src_h));
    FLT4 x3 = READ_IMAGE(src_data, smp_zero, (int2)((src_w + 1) * src_size.z + ci, src_h + 1));
    FLT16 weight_cache = weight[base_w++];
    r0 += x0.x * weight_cache.s0123;
    r0 += x0.y * weight_cache.s4567;
    r0 += x0.z * weight_cache.s89ab;
    r0 += x0.w * weight_cache.scdef;

    r1 += x1.x * weight_cache.s0123;
    r1 += x1.y * weight_cache.s4567;
    r1 += x1.z * weight_cache.s89ab;
    r1 += x1.w * weight_cache.scdef;

    r2 += x2.x * weight_cache.s0123;
    r2 += x2.y * weight_cache.s4567;
    r2 += x2.z * weight_cache.s89ab;
    r2 += x2.w * weight_cache.scdef;

    r3 += x3.x * weight_cache.s0123;
    r3 += x3.y * weight_cache.s4567;
    r3 += x3.z * weight_cache.s89ab;
    r3 += x3.w * weight_cache.scdef;
  }
  FLT4 bias_val = READ_IMAGE(biases, smp_zero, (int2)(co, 0));
  r0 += bias_val;
  r1 += bias_val;
  r2 += bias_val;
  r3 += bias_val;

  WRITE_IMAGE(dst_data, (int2)((2 * src_w + kw) * dst_size.z + co, 2 * src_h + kh), r0);
  WRITE_IMAGE(dst_data, (int2)((2 * src_w + kw) * dst_size.z + co, 2 * src_h + kh + 2), r1);
  WRITE_IMAGE(dst_data, (int2)((2 * src_w + kw + 2) * dst_size.z + co, 2 * src_h + kh), r2);
  WRITE_IMAGE(dst_data, (int2)((2 * src_w + kw + 2) * dst_size.z + co, 2 * src_h + kh + 2), r3);
}

__kernel void conv2d_transpose2x2_NC4HW4(__read_only image2d_t src_data, __global FLT16 *weight,
                                         __read_only image2d_t biases, __write_only image2d_t dst_data,
                                         int2 kernel_size, int2 stride, int2 padding, int4 src_size, int4 dst_size) {
  int h = get_global_id(0);
  int kh = h % 2;
  int src_h = h / 2;
  src_h = src_h * 2;
  int w = get_global_id(1);
  int kw = w % 2;
  int src_w = w / 2;
  src_w = src_w * 2;
  int co = get_global_id(2);
  if (src_h * 2 >= dst_size.x || src_w * 2 >= dst_size.y || co >= dst_size.z) return;
  FLT4 r0 = (FLT4)(0.f);
  FLT4 r1 = (FLT4)(0.f);
  FLT4 r2 = (FLT4)(0.f);
  FLT4 r3 = (FLT4)(0.f);
  int base_w = (co * 4 + kh * 2 + kw) * src_size.z;
  for (int ci = 0; ci < src_size.z; ++ci) {
    FLT4 x0 = READ_IMAGE(src_data, smp_zero, (int2)(src_w, ci * src_size.x + src_h));
    FLT4 x1 = READ_IMAGE(src_data, smp_zero, (int2)(src_w, ci * src_size.x + src_h + 1));
    FLT4 x2 = READ_IMAGE(src_data, smp_zero, (int2)(src_w + 1, ci * src_size.x + src_h));
    FLT4 x3 = READ_IMAGE(src_data, smp_zero, (int2)(src_w + 1, ci * src_size.x + src_h + 1));
    FLT16 weight_cache = weight[base_w++];
    r0 += x0.x * weight_cache.s0123;
    r0 += x0.y * weight_cache.s4567;
    r0 += x0.z * weight_cache.s89ab;
    r0 += x0.w * weight_cache.scdef;

    r1 += x1.x * weight_cache.s0123;
    r1 += x1.y * weight_cache.s4567;
    r1 += x1.z * weight_cache.s89ab;
    r1 += x1.w * weight_cache.scdef;

    r2 += x2.x * weight_cache.s0123;
    r2 += x2.y * weight_cache.s4567;
    r2 += x2.z * weight_cache.s89ab;
    r2 += x2.w * weight_cache.scdef;

    r3 += x3.x * weight_cache.s0123;
    r3 += x3.y * weight_cache.s4567;
    r3 += x3.z * weight_cache.s89ab;
    r3 += x3.w * weight_cache.scdef;
  }
  FLT4 bias_val = READ_IMAGE(biases, smp_zero, (int2)(co, 0));
  r0 += bias_val;
  r1 += bias_val;
  r2 += bias_val;
  r3 += bias_val;

  WRITE_IMAGE(dst_data, (int2)(2 * src_w + kw, co * dst_size.x + 2 * src_h + kh), r0);
  WRITE_IMAGE(dst_data, (int2)(2 * src_w + kw, co * dst_size.x + 2 * src_h + kh + 2), r1);
  WRITE_IMAGE(dst_data, (int2)(2 * src_w + kw + 2, co * dst_size.x + 2 * src_h + kh), r2);
  WRITE_IMAGE(dst_data, (int2)(2 * src_w + kw + 2, co * dst_size.x + 2 * src_h + kh + 2), r3);
}

__kernel void conv2d_transpose_NHWC4(__read_only image2d_t src_data, __global FLT16 *weight,
                                     __read_only image2d_t biases, __write_only image2d_t dst_data, int2 kernel_size,
                                     int2 stride, int2 padding, int4 src_size, int4 dst_size) {
  int dst_h = get_global_id(0);
  int rem_h = dst_h % stride.x;
  int ceil_h = dst_h / stride.x;
  dst_h = ceil_h * stride.x * 2 + rem_h;
  int dst_w = get_global_id(1);
  int rem_w = dst_w % stride.y;
  int ceil_w = dst_w / stride.y;
  dst_w = ceil_w * stride.y * 2 + rem_w;
  int dst_c = get_global_id(2);
  if (dst_h >= dst_size.x || dst_w >= dst_size.y || dst_c >= dst_size.z) return;
  int weight_base = dst_c * src_size.z * kernel_size.x * kernel_size.y;
  FLT4 r0 = (FLT4)(0.f);
  FLT4 r1 = (FLT4)(0.f);
  FLT4 r2 = (FLT4)(0.f);
  FLT4 r3 = (FLT4)(0.f);
  int kh_start = dst_h + padding.x;
  int kw_start = dst_w + padding.y;
  int kh_end = kh_start - kernel_size.x;
  int kw_end = kw_start - kernel_size.y;
  int src_h = kh_start / stride.x;
  int kh = src_h * stride.x;
  int src_w = kw_start / stride.y;
  int kw = src_w * stride.y;
  for (; kh > kh_end; src_h -= 1, kh -= stride.x) {
    int out0_src_h = src_h;
    int out1_src_h = src_h + 1;
    int kernel_h = kh_start - kh;
    int src_w_copy = src_w;
    int kw_copy = kw;
    for (; kw_copy > kw_end; src_w_copy -= 1, kw_copy -= stride.y) {
      int out0_src_w = src_w_copy;
      int out1_src_w = src_w_copy + 1;
      int kernel_w = kw_start - kw_copy;
      int weight_offset = weight_base + (kernel_h * kernel_size.y + kernel_w) * src_size.z;
      for (int ci = 0; ci < src_size.z; ++ci) {
        FLT4 x0 = READ_IMAGE(src_data, smp_zero, (int2)(out0_src_w * src_size.z + ci, out0_src_h));
        FLT4 x1 = READ_IMAGE(src_data, smp_zero, (int2)(out0_src_w * src_size.z + ci, out1_src_h));
        FLT4 x2 = READ_IMAGE(src_data, smp_zero, (int2)(out1_src_w * src_size.z + ci, out0_src_h));
        FLT4 x3 = READ_IMAGE(src_data, smp_zero, (int2)(out1_src_w * src_size.z + ci, out1_src_h));
        FLT16 weight_cache = weight[weight_offset++];
        r0 += x0.x * weight_cache.s0123;
        r0 += x0.y * weight_cache.s4567;
        r0 += x0.z * weight_cache.s89ab;
        r0 += x0.w * weight_cache.scdef;

        r1 += x1.x * weight_cache.s0123;
        r1 += x1.y * weight_cache.s4567;
        r1 += x1.z * weight_cache.s89ab;
        r1 += x1.w * weight_cache.scdef;

        r2 += x2.x * weight_cache.s0123;
        r2 += x2.y * weight_cache.s4567;
        r2 += x2.z * weight_cache.s89ab;
        r2 += x2.w * weight_cache.scdef;

        r3 += x3.x * weight_cache.s0123;
        r3 += x3.y * weight_cache.s4567;
        r3 += x3.z * weight_cache.s89ab;
        r3 += x3.w * weight_cache.scdef;
      }
    }
  }
  FLT4 bias_val = READ_IMAGE(biases, smp_zero, (int2)(dst_c, 0));
  r0 += bias_val;
  r1 += bias_val;
  r2 += bias_val;
  r3 += bias_val;
  WRITE_IMAGE(dst_data, (int2)(dst_w * dst_size.z + dst_c, dst_h), r0);
  if (dst_h + stride.x < dst_size.x && dst_w < dst_size.y) {
    WRITE_IMAGE(dst_data, (int2)(dst_w * dst_size.z + dst_c, dst_h + stride.x), r1);
  }
  if (dst_h < dst_size.x && dst_w + stride.y < dst_size.y) {
    WRITE_IMAGE(dst_data, (int2)((dst_w + stride.y) * dst_size.z + dst_c, dst_h), r2);
  }
  if (dst_h + stride.x < dst_size.x && dst_w + stride.y < dst_size.y) {
    WRITE_IMAGE(dst_data, (int2)((dst_w + stride.y) * dst_size.z + dst_c, dst_h + stride.x), r3);
  }
}
