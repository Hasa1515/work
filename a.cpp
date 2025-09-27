
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define rand1() ((double)rand()/RAND_MAX)
#define N 1000
#define TS 0.0001

int main()
{
    int n, k;
    double xr[N], XR[N], XI[N], pi;
    double amp[N], phase[N], xr_re[N];  // 振幅・位相スペクトル、復元信号
    
    pi = acos(-1.0);
    srand(42);  // 再現性のためのシード固定
    
    // パルス波パラメータ
    double pulse_freq = 50.0;         // パルス周波数 [Hz]
    double pulse_amplitude = 2.5;     // パルス振幅
    double duty_cycle = 0.3;          // デューティ比（30%）
    double dc_offset = 0.8;           // DC オフセット
    double noise_level = 0.05;        // ノイズレベル
    
    printf("=== Pulse Wave Parameters ===\n");
    printf("Frequency: %.1f Hz\n", pulse_freq);
    printf("Amplitude: %.2f V\n", pulse_amplitude);
    printf("Duty Cycle: %.1f%%\n", duty_cycle * 100);
    printf("DC Offset: %.2f V\n", dc_offset);
    printf("Noise Level: %.3f V\n", noise_level);
    printf("Sampling Rate: %.0f Hz\n", 1.0/TS);
    printf("==================================\n\n");
    
    // 入力信号の生成（パルス波）
    for (n = 0; n < N; n++)
    {
        double t_val = TS * n;
        double period = 1.0 / pulse_freq;
        double time_in_period = fmod(t_val, period);
        
        // パルス波の生成
        double pulse_signal;
        if (time_in_period < (duty_cycle * period)) {
            pulse_signal = pulse_amplitude;  // パルス部分（High）
        } else {
            pulse_signal = 0.0;              // 非パルス部分（Low）
        }
        // ノイズとDCオフセットを追加
        double noise = noise_level * (rand1() - 0.5);
        xr[n] = pulse_signal + dc_offset + noise;
    }
    // DFT計算
    printf("Computing DFT...\n");
    for (k = 0; k < N; k++)
    {
        XR[k] = 0.0;
        XI[k] = 0.0;
        for (n = 0; n < N; n++)
        {
            double angle = 2.0 * pi * n * k / N;
            XR[k] = XR[k] + xr[n] * cos(angle);
            XI[k] = XI[k] - xr[n] * sin(angle);
        }
        // 振幅スペクトルと位相スペクトル
        amp[k] = sqrt(XR[k] * XR[k] + XI[k] * XI[k]);
        phase[k] = atan2(-XI[k], XR[k]);
    }
    // IDFT計算（信号復元）
    printf("Computing IDFT...\n");
    for (n = 0; n < N; n++)
    {
        double sum = 0.0;
        for (k = 0; k < N; k++)
        {
            double angle = 2.0 * pi * n * k / N;
            sum += XR[k] * cos(angle) - XI[k] * sin(angle);
        }
        xr_re[n] = sum / N;
    }
    
    // 結果出力
    printf("\n=== Time Domain Samples (First 15 points) ===\n");
    printf("n\tOriginal\tReconstructed\tError\n");
    printf("----------------------------------------------\n");
    for (k = 0; k < 15; k++)
    {
        double error = fabs(xr[k] - xr_re[k]);
        printf("%d\t%.6f\t%.6f\t\t%.8f\n", k, xr[k], xr_re[k], error);
    }
    
    // DFT結果（周波数域）
    printf("\n=== DFT Results (Selected points) ===\n");
    printf("k\tFreq[Hz]\tRe(DFT)\t\tIm(DFT)\t\tAmplitude\tPhase[rad]\n");
    printf("------------------------------------------------------------------------\n");
    for (k = 0; k < 20; k++)
    {
        double freq = (double)k / (N * TS);
        printf("%d\t%.2f\t\t%.4f\t\t%.4f\t\t%.4f\t\t%.4f\n", 
               k, freq, XR[k], XI[k], amp[k], phase[k]);
    }
    // 主要な周波数成分の解析
    printf("\n=== Major Frequency Components ===\n");
    printf("Frequency[Hz]\tAmplitude\tPhase[rad]\tComponent\n");
    printf("------------------------------------------------\n");
    
    double fundamental_freq = pulse_freq;
    for (k = 0; k < N/2; k++)
    {
        double freq = (double)k / (N * TS);
        if (amp[k] > 20.0)  // 閾値以上の成分のみ表示
        {
            if (k == 0) {
                printf("%.2f\t\t%.2f\t\t%.4f\t\tDC Component\n", 
                       freq, amp[k], phase[k]);
            } else {
                int harmonic = (int)round(freq / fundamental_freq);
                if (fabs(freq - harmonic * fundamental_freq) < 2.0) {
                    printf("%.2f\t\t%.2f\t\t%.4f\t\t%d次高調波\n", 
                           freq, amp[k], phase[k], harmonic);
                } else {
                    printf("%.2f\t\t%.2f\t\t%.4f\t\tOther\n", 
                           freq, amp[k], phase[k]);
                }
            }
        }
    }
    // 復元精度の評価
    printf("\n=== Reconstruction Quality ===\n");
    double max_error = 0.0;
    double rms_error = 0.0;
    for (n = 0; n < N; n++)
    {
        double error = fabs(xr[n] - xr_re[n]);
        if (error > max_error) max_error = error;
        rms_error += error * error;
    }
    rms_error = sqrt(rms_error / N);
    printf("Maximum Error: %.10f\n", max_error);
    printf("RMS Error: %.10f\n", rms_error);
    // 理論値との比較
    printf("\n=== Theoretical Analysis ===\n");
    printf("理論DC成分: %.4f V\n", pulse_amplitude * duty_cycle + dc_offset);
    printf("実測DC成分: %.4f V\n", XR[0] / N);
    printf("基本波理論振幅: %.4f V\n", 2.0 * pulse_amplitude * duty_cycle);
    // 基本波成分の検索
    int fundamental_index = (int)round(fundamental_freq * N * TS);
    if (fundamental_index < N) {
        printf("基本波実測振幅: %.4f V\n", amp[fundamental_index]);
    }
    return 0;
}