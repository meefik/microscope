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
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

public class Microscope extends Activity {

	static Handler handler;
	private static TextView logView;
	private static TextView tv;
	private static ImageView image;

	private final int imgSize = 50000;
	private boolean isActive = false;

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
					showStatus(capture(mImageData, size));

					mImageData.get(bytes, 0, size);
					final Bitmap bMap = BitmapFactory.decodeByteArray(bytes, 0,
							size, options);

					handler.post(new Runnable() {
						public void run() {
							image.setImageBitmap(bMap);
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
		image = (ImageView) findViewById(R.id.imageView1);

		final Button captureBtn = (Button) findViewById(R.id.saveBtn);
		captureBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				File root = Environment.getExternalStorageDirectory();
				SharedPreferences sp = PreferenceManager
						.getDefaultSharedPreferences(getApplicationContext());
				String imgPath = sp.getString("imgpath", root.getAbsolutePath());
				image.setDrawingCacheEnabled(true);
				Bitmap bitmap = image.getDrawingCache();
				String fName = "microscope_"
						+ new SimpleDateFormat("yyMMddHHmmss", Locale.US)
								.format(new Date()) + ".jpg";
				File file = new File(imgPath + "/" + fName);
				try {
					file.createNewFile();
					FileOutputStream ostream = new FileOutputStream(file);
					bitmap.compress(CompressFormat.JPEG, 100, ostream);
					ostream.close();
					showStatus("Saved: " + fName);
				} catch (Exception e) {
					e.printStackTrace();
					showStatus("Save error");
				}
				image.setDrawingCacheEnabled(false);
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

	public static void printStatusMsg(String msg) {
		logView.setText(msg);
	}

	static {
		System.loadLibrary("microscope");
	}

	public native String close();

	public native String open(String device);

	public native String capture(ByteBuffer globalRef, long imgSize);

	public native ByteBuffer allocNativeBuffer(long size);

	public native void freeNativeBuffer(ByteBuffer globalRef);

}
