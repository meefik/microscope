package ru.lomo.microscope;

import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;

import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.BitmapFactory.Options;
import android.util.Log;
import android.widget.ImageView;

public class Microscope extends Activity {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_microscope);

        // Загрузки файла с флешки
        final Button open_button = (Button) findViewById(R.id.open_button);
        open_button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                // Perform action on click
            	ImageView image = (ImageView) findViewById(R.id.imageView1);   
                FileInputStream in;
                BufferedInputStream buf;
                try {
               	    in = new FileInputStream("/sdcard/video.jpg");
                    buf = new BufferedInputStream(in);
                    Options options = new BitmapFactory.Options();
                    options.inSampleSize = 1;
                    Bitmap bMap = BitmapFactory.decodeStream(buf, null, options);
                    image.setImageBitmap(bMap);
                    if (in != null) {
                 	in.close();
                    }
                    if (buf != null) {
                 	buf.close();
                    }
                } catch (Exception e) {
                    Log.e("Error reading file", e.toString());
                }
            }
        });
        
        // Получение буффера с камеры
        final Button capture_button = (Button) findViewById(R.id.capture_button);
        capture_button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                // Perform action on click
            	final TextView tv = (TextView) findViewById(R.id.textView1);
            	final ImageView image = (ImageView) findViewById(R.id.imageView1);
            	
            	ByteBuffer mImageData = (ByteBuffer) allocNativeBuffer(640 * 480);
            	byte[] bytes = new byte[mImageData.capacity()];

                tv.setText( capture(mImageData) );
                
            	mImageData.get(bytes, 0, bytes.length);

                Options options = new BitmapFactory.Options();
                options.inSampleSize = 1;
            	
            	Bitmap bMap = BitmapFactory.decodeByteArray(bytes, 0, 640 * 480, options);
            	image.setImageBitmap(bMap);

                // Сохранение файла на флешку (опционально)
                try {
                    FileOutputStream out = new FileOutputStream("/sdcard/video.jpg");
                    bMap.compress(Bitmap.CompressFormat.JPEG, 90, out);
                } catch (Exception e) {
                    e.printStackTrace();
                }
                
                freeNativeBuffer(mImageData);
                mImageData = null;
            }
        });        
        
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.activity_microscope, menu);
        return true;
    }
    
    static {
    	System.loadLibrary("microscope");
	}
	  
	public native String open();
	public native String close();
	public native String capture(ByteBuffer globalRef);
	public native ByteBuffer allocNativeBuffer(long size);
	public native void freeNativeBuffer(ByteBuffer globalRef);
	
}
