// Created by ruoyi.sjd on 2024/12/25.
// Copyright (c) 2024 Alibaba Group Holding Limited All rights reserved.
package com.alibaba.mls.api.hf

import android.util.Log
import okhttp3.Interceptor
import okhttp3.Response
import okhttp3.ResponseBody
import okio.Buffer
import java.io.IOException

class LoggingInterceptor : Interceptor {
    companion object {
        private const val TAG = "NetworkLog"
    }

    @Throws(IOException::class)
    override fun intercept(chain: Interceptor.Chain): Response {
        val request = chain.request()
        
        // Log request details
        val requestBuffer = Buffer()
        request.body?.writeTo(requestBuffer)
        val requestBody = requestBuffer.readUtf8()
        
        Log.d(TAG, "=== REQUEST ===")
        Log.d(TAG, "Method: ${request.method}")
        Log.d(TAG, "URL: ${request.url}")
        Log.d(TAG, "Headers: ${request.headers}")
        Log.d(TAG, "Body: $requestBody")
        Log.d(TAG, "===============")
        
        val startTime = System.currentTimeMillis()
        val response = chain.proceed(request)
        val endTime = System.currentTimeMillis()
        
        // Log response details
        val responseBody = response.body
        val responseBodyString = responseBody?.string() ?: ""
        
        Log.d(TAG, "=== RESPONSE ===")
        Log.d(TAG, "Time taken: ${endTime - startTime} ms")
        Log.d(TAG, "Status Code: ${response.code}")
        Log.d(TAG, "Message: ${response.message}")
        Log.d(TAG, "Headers: ${response.headers}")
        Log.d(TAG, "Body: $responseBodyString")
        Log.d(TAG, "================")
        
        // Return a new response with the original body
        return response.newBuilder()
            .body(ResponseBody.create(responseBody?.contentType(), responseBodyString))
            .build()
    }
}