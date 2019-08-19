package com.example.androidex;

import com.example.androidex.MainActivity2.BackThread;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;

public class MyService extends Service {
	
	int seconds = 0;
    IBinder mBinder = new MyBinder();
	
    class MyBinder extends Binder{
        MyService getService(){
            return MyService.this;
        }
    }
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    	//return null;
    }
    
    int getTime(){
    	seconds++;
    	return seconds;
    }
    
    @Override
    public void onCreate() {
        super.onCreate();
    }

    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public boolean onUnbind(Intent intent) {
        return super.onUnbind(intent);
    }
}