package com.yuong.ffmpegplayer.player;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class FFmpegPlayer implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("player");
    }

    private SurfaceHolder holder;
    private String dataSource;
    private OnPrepareListener onPrepareListener;
    private OnProgressListener onProgressListener;
    private OnErrorListener onErrorListener;

    public void setSurfaceView(SurfaceView surfaceView) {
        if (this.holder != null) {
            this.holder.removeCallback(this);
            this.holder = null;
        }
        this.holder = surfaceView.getHolder();
        this.holder.addCallback(this);
    }

    public void prepare() {
        native_prepare(dataSource);
    }

    public void start() {
        native_start();
    }


    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        native_setSurface(holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }

    public void setOnPrepareListener(OnPrepareListener onPrepareListener) {
        this.onPrepareListener = onPrepareListener;
    }

    public void setOnProgressListener(OnProgressListener onProgressListener) {
        this.onProgressListener = onProgressListener;
    }

    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    public void onPrepare() {
        if (onPrepareListener != null) {
            onPrepareListener.onPrepare();
        }
    }

    public void onError(int errorCode) {
        if (onErrorListener != null) {
            onErrorListener.onError(errorCode);
        }
    }

    public void onProgress(int progress) {
        if (onProgressListener != null) {
            onProgressListener.onProgress(progress);
        }
    }


    public interface OnPrepareListener {
        void onPrepare();
    }

    public interface OnErrorListener {
        void onError(int errorCode);
    }

    public interface OnProgressListener {
        void onProgress(int progress);
    }


    /**
     * 准备
     *
     * @param dataSource 数据源
     */
    private native void native_prepare(String dataSource);

    /**
     * 设置Surface
     *
     * @param surface
     */
    private native void native_setSurface(Surface surface);

    /**
     * 开始
     */
    private native void native_start();

}
