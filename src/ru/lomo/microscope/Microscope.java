package ru.lomo.microscope;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.graphics.BitmapFactory.Options;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.FloatMath;
import android.view.GestureDetector;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.TextView;

public class Microscope extends Activity implements OnTouchListener {

	static Handler handler;
	private static TextView logView;
	private static TextView tv;
	private static ImageView iv;

	private final int imgSize = 50000;
	private boolean isActive = false;
	
	private boolean rotateState = false;
	
	// These matrices will be used to move and zoom image
	Matrix matrix = new Matrix();
	Matrix savedMatrix = new Matrix();
	
	// We can be in one of these 3 states
    static final int NONE = 0;
	static final int DRAG = 1;
	static final int ZOOM = 2;
	int mode = NONE;
	
	// Remember some things for zooming
	PointF start = new PointF();
	PointF mid = new PointF();
	float oldDist = 1f;
	
    //creates a Gesture Detector  
    private GestureDetector gd; 

	static {
		System.loadLibrary("microscope");
	}

	public native ByteBuffer allocNativeBuffer(long size);

	public native String capture(ByteBuffer globalRef, long imgSize);

	public native String open(String device);
	
	public native String get();

	public native String set(int brightness, int hue, int colour, 
								int contrast, int whiteness, int depth, int palette);
	
	public native String close();

	public native void freeNativeBuffer(ByteBuffer globalRef);

	public static void printStatusMsg(String msg) {
		logView.setText(msg);
	}

	private void showStatus(final String msg) {
		handler.post(new Runnable() {
			public void run() {
				tv.setText(msg);
			}
		});
	}

	private void captureExec(final String device, final int size) {
		if (isActive)
			return;
		isActive = true;
		Runnable ex = new Runnable() {
			public void run() {
				// open
				ByteBuffer mImageData = (ByteBuffer) allocNativeBuffer(size);
				showStatus(open(device));
				byte[] bytes = new byte[size];
				final Options options = new BitmapFactory.Options();
				options.inSampleSize = 1;
				while (isActive) {
					// for (int i=0; i < 3; i++) {
					mImageData.clear();
					capture(mImageData, size);
					//showStatus(capture(mImageData, size));

					mImageData.get(bytes, 0, size);
					final Bitmap bMap = BitmapFactory.decodeByteArray(bytes, 0,
							size, options);

					handler.post(new Runnable() {
						public void run() {
							iv.setImageBitmap(bMap);
						}
					});
				}
				// close
				showStatus(close());
				freeNativeBuffer(mImageData);
				mImageData = null;
			}
		};
		Thread thread = new Thread(ex, "capture");
		thread.start();
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_microscope);

		handler = new Handler();
		tv = (TextView) findViewById(R.id.textView1);
		iv = (ImageView) findViewById(R.id.imageView1);
		
		iv.setScaleType(ScaleType.MATRIX);

		final Button saveBtn = (Button) findViewById(R.id.saveBtn);
		saveBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				File root = Environment.getExternalStorageDirectory();
				SharedPreferences sp = PreferenceManager
						.getDefaultSharedPreferences(getApplicationContext());
				String imgPath = sp.getString("imgpath", root.getAbsolutePath());
				iv.setDrawingCacheEnabled(true);
				Bitmap bitmap = iv.getDrawingCache();
				String fName = "micro_"
						+ new SimpleDateFormat("yyMMddHHmmss", Locale.US)
								.format(new Date()) + ".jpg";
				File file = new File(imgPath + "/" + fName);
				try {
					file.createNewFile();
					FileOutputStream ostream = new FileOutputStream(file);
					bitmap.compress(CompressFormat.JPEG, 100, ostream);
					ostream.close();
					showStatus("Saved: " + file.getPath());
				} catch (Exception e) {
					e.printStackTrace();
					showStatus("Save error");
				}
				iv.setDrawingCacheEnabled(false);
			}
		});
		final Button startBtn = (Button) findViewById(R.id.startBtn);
		startBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				SharedPreferences sp = PreferenceManager
						.getDefaultSharedPreferences(getBaseContext());
				String devPath = sp.getString("devpath", "/dev/video0");
				captureExec(devPath, imgSize);
			}
		});
		final Button stopBtn = (Button) findViewById(R.id.stopBtn);
		stopBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				isActive = false;
			}
		});
		
		final Button flipBtn = (Button) findViewById(R.id.flipBtn);
		flipBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				float rotate;
				if (rotateState) {
					rotate = 0f;
					rotateState = false;
				} else {
					rotate = 180f;
					rotateState = true;
				}
				//matrix.postRotate(rotate, iv.getDrawable().getBounds().width()/2, iv.getDrawable().getBounds().height()/2);
				matrix.setRotate(rotate, iv.getDrawable().getBounds().width()/2, iv.getDrawable().getBounds().height()/2);
				iv.setImageMatrix(matrix);
			}
		});

		final Button getBtn = (Button) findViewById(R.id.getBtn);
		getBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				SharedPreferences sp = PreferenceManager
						.getDefaultSharedPreferences(getApplicationContext());
				SharedPreferences.Editor prefEditor = sp.edit();
				String[] mKey = { "brightness","hue","colour","contrast",
						"whiteness","depth","palette" };
				String[] mVal = get().split(";");
				for (int i = 0; i < 7; i++) {
					prefEditor.putString(mKey[i], mVal[i]);
				}
				prefEditor.commit();
			}
		});
		
		final Button setBtn = (Button) findViewById(R.id.setBtn);
		setBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				SharedPreferences sp = PreferenceManager
						.getDefaultSharedPreferences(getApplicationContext());
				int brightness, hue, colour, contrast, whiteness, depth, palette;
				brightness = Integer.parseInt(sp.getString("brightness", "0"));
				hue = Integer.parseInt(sp.getString("hue", "0"));
				colour = Integer.parseInt(sp.getString("colour", "0"));
				contrast = Integer.parseInt(sp.getString("contrast", "0"));
				whiteness = Integer.parseInt(sp.getString("whiteness", "0"));
				depth = Integer.parseInt(sp.getString("depth", "0"));
				palette = Integer.parseInt(sp.getString("palette", "0"));
				showStatus(set(brightness,hue,colour,contrast,whiteness,depth,palette));
			}
		});
		
		iv.setOnTouchListener(this);
		
        //initialize the Gesture Detector  
        gd = new GestureDetector(this, new GestureDetector.SimpleOnGestureListener()  
        {  
            @Override  
            public boolean onDoubleTap(MotionEvent e)  
            {  
            	matrix.reset();
            	savedMatrix.reset();
                return false;  
            }  
        });  
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.activity_microscope, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.menu_settings:
			Intent intent_pref = new Intent(this, PreferencesActivity.class);
			startActivity(intent_pref);
			break;
		default:

			break;
		}
		return false;
	}

	@Override
	public void onPause() {
		super.onPause();
		isActive = false;
	}

	@Override
	public boolean onTouch(View v, MotionEvent event) {
	      ImageView view = (ImageView) v;

	      // Handle touch events here...
	      switch (event.getAction() & MotionEvent.ACTION_MASK) {
	      case MotionEvent.ACTION_DOWN:
	         savedMatrix.set(matrix);
	         start.set(event.getX(), event.getY());
	         mode = DRAG;
	         break;
	      case MotionEvent.ACTION_POINTER_DOWN:
	         oldDist = spacing(event);
	         if (oldDist > 10f) {
	            savedMatrix.set(matrix);
	            midPoint(mid, event);
	            mode = ZOOM;
	         }
	         break;
	      case MotionEvent.ACTION_UP:
	      case MotionEvent.ACTION_POINTER_UP:
	         mode = NONE;
	         break;
	      case MotionEvent.ACTION_MOVE:
	         if (mode == DRAG) {
	            matrix.set(savedMatrix);
	            //float newx = event.getX() - start.x;
	            //float newy = event.getY() - start.y;
	            //showStatus(String.valueOf(newx)+" "+String.valueOf(newy));
	            matrix.postTranslate(event.getX() - start.x,
	                  event.getY() - start.y);
	            
	         }
	         else if (mode == ZOOM) {
	            float newDist = spacing(event);
	            if (newDist > 10f) {
	               matrix.set(savedMatrix);
	               float scale = newDist / oldDist;
	               matrix.postScale(scale, scale, mid.x, mid.y);
	            }
	         }
	         break;
	      }

	      view.setImageMatrix(matrix);
	      
	      gd.onTouchEvent(event);
	      
	      return true;
	}
	
	// Determine the space between the first two fingers
	private float spacing(MotionEvent event) {
	    float x = event.getX(0) - event.getX(1);
	    float y = event.getY(0) - event.getY(1);
	    return FloatMath.sqrt(x * x + y * y);
	}

	// Calculate the mid point of the first two fingers
	private void midPoint(PointF point, MotionEvent event) {
		float x = event.getX(0) + event.getX(1);
	    float y = event.getY(0) + event.getY(1);
	    point.set(x / 2, y / 2);
	}

}
