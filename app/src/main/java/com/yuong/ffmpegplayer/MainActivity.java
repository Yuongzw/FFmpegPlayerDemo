package com.yuong.ffmpegplayer;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.SeekBar;

import androidx.appcompat.app.AppCompatActivity;

import com.yuong.ffmpegplayer.player.FFmpegPlayer;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    private FFmpegPlayer fmpegPlayer;
    private SeekBar seekBar;
    private int progress;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_main);
        SurfaceView surfaceView = findViewById(R.id.surfaceView);
        seekBar = findViewById(R.id.seekBar);
        checkPermissions();
        fmpegPlayer = new FFmpegPlayer();
        fmpegPlayer.setSurfaceView(surfaceView);
        File file = new File(Environment.getExternalStorageDirectory(), "input.mp4");
        fmpegPlayer.setDataSource(file.getAbsolutePath());
        fmpegPlayer.setOnPrepareListener(new FFmpegPlayer.OnPrepareListener() {
            @Override
            public void onPrepare() {
                fmpegPlayer.start();
            }
        });

    }

    private void checkPermissions() {
        boolean isGranted = true;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED){
                //如果没有存储权限
                isGranted = false;
            }
            if (!isGranted) {
                requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.ACCESS_BACKGROUND_LOCATION,
                Manifest.permission.ACCESS_FINE_LOCATION}, 666);
            }
        }
    }


    public void play(View view) {
        fmpegPlayer.prepare();
    }
}
