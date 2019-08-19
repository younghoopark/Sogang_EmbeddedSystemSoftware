package com.example.androidex;


import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;


import com.example.androidex.MyService;
import com.example.androidex.MainActivity2.BackThread;
import com.example.androidex.MyService.MyBinder;


public class MainActivity2 extends Activity{
	LinearLayout linear;
	EditText input_rc;
	
	/* variables */
	int row, col;
	int blank_;
	MyService ms;
	boolean isService = false;
	boolean serviceflag = false;
	boolean startflag = false;
	int seconds = 0;
	ServiceConnection conn = new ServiceConnection(){
		public void onServiceConnected(ComponentName name, IBinder service){
			MyBinder mb = (MyBinder)service;
			ms = mb.getService();
			isService = true;
		}
		
		public void onServiceDisconnected(ComponentName name){
			isService = false;
		}
	};

	Handler mHandler=new Handler(){
		public void handleMessage(Message msg){
			if(msg.what==0){
				//mBackText.setText("BackValue : "+msg.arg1);
				//mBackText.setText(String.format("%02d", msg.arg1)+":"+String.format("%02d", msg.arg2));
				String s1 = String.format("%02d", msg.arg1);
				String s2 = String.format("%02d", msg.arg2);
				mBackText.setText(s1+":"+s2);
				//mBackText.setText(msg.arg1+":"+msg.arg2);
			}
		}
	};
	class BackThread extends Thread{
		int mBackValue=0;
		Handler sHandler;
		
		BackThread(Handler handler){
			sHandler=handler;
		}
		public void run(){
			while(true){
				Message msg=Message.obtain();
				msg.what=0;
				if(serviceflag == true){
					seconds = ms.getTime();
					msg.arg1 = seconds / 60;
					msg.arg2 = seconds % 60;
				}
				sHandler.sendMessage(msg);
				try{Thread.sleep(1000);}catch(InterruptedException e){;}
			}
		}
	}
	
	TextView mBackText;
	BackThread mThread;
	
	/* METHOD NAME : onCreate */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main2);
		linear = (LinearLayout)findViewById(R.id.container);
		Button btn = (Button)findViewById(R.id.button1);
		input_rc = (EditText)findViewById(R.id.editText1);
		mBackText=(TextView)findViewById(R.id.backvalue);
		
		
		/* Define Button(Make Buttons) Listener */
		OnClickListener btnlsn = new OnClickListener(){
			public void onClick(View v){
				if(startflag == true){
					unbindService(conn);
					seconds = 0;
					serviceflag = false;
					startflag = false;
				}
				/* ABOUT PUZZLE CODE */
				String s = input_rc.getText().toString();
				
				row = Integer.parseInt(s.split(" ")[0]);
				col = Integer.parseInt(s.split(" ")[1]);
				if(row == 0 || col == 0){
					return;
				}
				if(row == 1 && col == 1){
					return;
				}
				if(row == 1 && col == 2){
					//return;
				}
				if(row == 2 && col == 1){
					//return;
				}
				if(row <= 5 && col <= 5){
					Intent intent = new Intent(MainActivity2.this, MyService.class);
					bindService(intent, conn, Context.BIND_AUTO_CREATE);
					serviceflag = true;
					startflag = true;
					Puzzle_make();
				}
			}
		};
		btn.setOnClickListener(btnlsn);
		
		mThread=new BackThread(mHandler);	//make new BackThread object, parameter is mHandler(set the textView2)
		mThread.setDaemon(true);
		mThread.start();	//mThread start
		
	}
	
	/* METHOD NAME : Puzzle_shuffle */
	protected void Puzzle_shuffle(){
		for(int i = 0; i < 77; i++){
			Puzzle_swap((int)(Math.random() * (row * col) + 1));
			if(row >= 2 && col >= 2){
				if(Puzzle_Complete() == true){
					i = 0;
				}
			}
		}	
	}
	

	/* METHOD NAME : Puzzle_make */
	protected void Puzzle_make(){
		int id;
		int laycol = 1024/col, layrow = 400/row;
		blank_ = row * col;	
		for(int i = 0; i < row; i++){
			LinearLayout rows = new LinearLayout(this);
			rows.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));

			
			for(int j = 0; j < col; j++){
				id = (i*col)+(j+1);
				
                /* make button */
				Button btn = new Button(this);
				btn.setLayoutParams(new LayoutParams(laycol, layrow));
				btn.setId(id);
				btn.setBackgroundColor(Color.GRAY);
				
				
                /* set Button's row & column */
				int[] btnpos = new int[2];
				btnpos[0] = i; btnpos[1] = j;
				btn.setTag(btnpos);
				btn.setText("" + id);
				
				if(id == row * col) {
					// Set blank
                    btn.setBackgroundColor(Color.BLACK);
				}

				OnClickListener listener2 = new OnClickListener(){
					public void onClick(View v){
                        if(Puzzle_swap(v.getId()) && Puzzle_Complete()){ 
                        	unbindService(conn);
                    		seconds = 0;
                    		serviceflag = false;
                    		startflag = false;
                    		finish();
                        }
					}
				};
				btn.setOnClickListener(listener2);
				
				rows.addView(btn);
			}
			linear.addView(rows);
		}
		
		Puzzle_shuffle();
	}

	
	
	/* METHOD NAME : Puzzle_swap */
	protected boolean Puzzle_swap(int id){
		Button blank = (Button)findViewById(blank_);
		Button target = (Button)findViewById(id);
		
		int[] blanktag = (int [])blank.getTag(); String blanktext;
		int[] targettag = (int [])target.getTag(); String targettext;
		
		
		boolean changeflag = false;
	
		/* UP */
		if(blanktag[0] == targettag[0]-1 && blanktag[1] == targettag[1]){
			changeflag = true;
		}
		/* DOWN */
		else if(blanktag[0] == targettag[0]+1 && blanktag[1] == targettag[1]){
			changeflag = true;
		}
		/* LEFT */
		else if(blanktag[0] == targettag[0] && blanktag[1] == targettag[1]-1){
			changeflag = true;
		}
		/* RIGHT */
		else if(blanktag[0] == targettag[0] && blanktag[1] == targettag[1]+1){
			changeflag = true;
		}
		
		/* CHANGE Blank and Target*/
		if(changeflag == true){
			blank_ = target.getId();
			blanktext = blank.getText().toString();	targettext = target.getText().toString();
			target.setText(blanktext); target.setBackgroundColor(Color.BLACK);
			blank.setText(targettext); blank.setBackgroundColor(Color.GRAY);
		}
		return changeflag;
	}


	protected boolean Puzzle_Complete(){
		if (blank_ != row * col){
			return false;
		}		
		/* Check the Puzzle is complete */
		for(int i = 1; i <= row * col; i++){
			Button btn = (Button)findViewById(i);
			int[] btnpos = (int [])btn.getTag();
			int puzzle_textvalue = Integer.parseInt(btn.getText().toString());
			if(puzzle_textvalue != (btnpos[1]+1)+(btnpos[0]*col)){
				return false;
			}
		}
		return true;
	}

}

