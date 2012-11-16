package ru.lomo.microscope;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.Environment;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;

public class PreferencesActivity extends PreferenceActivity implements
		OnSharedPreferenceChangeListener, Preference.OnPreferenceClickListener {

	private String FILES_DIR;

	private boolean copyFile(String filename) {
		boolean result = true;
		AssetManager assetManager = getBaseContext().getAssets();
		InputStream in = null;
		OutputStream out = null;
		try {
			SharedPreferences sp = PreferenceManager
					.getDefaultSharedPreferences(getBaseContext());
			String platform = sp.getString("platform", "edge");
			in = assetManager.open("drivers/" + platform + "/" + filename);
			String newFileName = FILES_DIR + File.separator + filename;
			out = new FileOutputStream(newFileName);

			byte[] buffer = new byte[1024];
			int read;
			while ((read = in.read(buffer)) != -1) {
				out.write(buffer, 0, read);
			}
			in.close();
			in = null;
			out.flush();
			out.close();
			out = null;
		} catch (IOException e) {
			e.printStackTrace();
			result = false;
		} finally {
			if (in != null) {
				try {
					in.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
			if (out != null) {
				try {
					out.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}
		return result;
	}

	private void extractData() {
		copyFile("gspca_main.ko");
		copyFile("gspca_zc3xx.ko");
	}

	private void initSummaries(PreferenceGroup pg) {
		for (int i = 0; i < pg.getPreferenceCount(); ++i) {
			Preference p = pg.getPreference(i);
			if (p instanceof PreferenceGroup)
				this.initSummaries((PreferenceGroup) p);
			else
				this.setSummary(p, false);
			if (p instanceof PreferenceScreen)
				p.setOnPreferenceClickListener(this);
		}
	}
	
	private void setSummary(Preference pref, boolean init) {
		if (pref instanceof EditTextPreference) {
			EditTextPreference editPref = (EditTextPreference) pref;
			pref.setSummary(editPref.getText());
		}

		if (pref instanceof ListPreference) {
			ListPreference listPref = (ListPreference) pref;
			pref.setSummary(listPref.getEntry());
		}
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		this.addPreferencesFromResource(R.xml.preferences);
		this.initSummaries(this.getPreferenceScreen());
		FILES_DIR = getFilesDir().getAbsolutePath();
	}

	@Override
	public void onPause() {
		super.onPause();
		this.getPreferenceScreen().getSharedPreferences()
				.unregisterOnSharedPreferenceChangeListener(this);
	}

	public boolean onPreferenceClick(Preference preference) {
		if (preference.getKey().equals("installdrv")) {
			installDrvDialog();
		}
		if (preference.getKey().equals("removedrv")) {
			removeDrvDialog();
		}
		return true;
	}

	@Override
	public void onResume() {
		super.onResume();
		this.getPreferenceScreen().getSharedPreferences()
				.registerOnSharedPreferenceChangeListener(this);
	}

	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
			String key) {
		Preference pref = this.findPreference(key);
		this.setSummary(pref, true);
	}
	
	private void installDrvDialog() {
		new AlertDialog.Builder(this)
				.setTitle(R.string.title_installdrv_preference)
				.setMessage(R.string.message_installdrv_confirm_dialog)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setCancelable(false)
				.setPositiveButton(android.R.string.yes,
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int id) {
								extractData();
								List<String> params = new ArrayList<String>();
								params.add("su");
								params.add("mount -o rw,remount /system");
								params.add("cp "
										+ FILES_DIR
										+ "/gspca_main.ko /system/lib/modules/gspca_main.ko");
								params.add("cp "
										+ FILES_DIR
										+ "/gspca_zc3xx.ko /system/lib/modules/gspca_zc3xx.ko");
								params.add("chmod 644 /system/lib/modules/gspca_main.ko");
								params.add("chmod 644 /system/lib/modules/gspca_zc3xx.ko");
								params.add("chown root:root /system/lib/modules/gspca_main.ko");
								params.add("chown root:root /system/lib/modules/gspca_zc3xx.ko");
								params.add("mount -o ro,remount /system");
								params.add("insmod /system/lib/modules/gspca_main.ko");
								params.add("insmod /system/lib/modules/gspca_zc3xx.ko");
								params.add("exit");
								ExecCmd ex = new ExecCmd(params);
								ex.run();
								final String msg;
								if (!ex.status) {
									// Log.d("microscope","Install FAIL!");
									msg = "Instal driver fail!";
								} else {
									msg = "Instal driver OK";
								}
								Microscope.handler.post(new Runnable() {
									public void run() {
										Microscope.printStatusMsg(msg);
									}
								});
								finish();
							}
						})
				.setNegativeButton(android.R.string.no,
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int id) {
								dialog.cancel();
							}
						}).show();
	}

	private void removeDrvDialog() {
		new AlertDialog.Builder(this)
				.setTitle(R.string.title_removedrv_preference)
				.setMessage(R.string.message_removedrv_confirm_dialog)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setCancelable(false)
				.setPositiveButton(android.R.string.yes,
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int id) {
								List<String> params = new ArrayList<String>();
								params.add("su");
								params.add("rmmod /system/lib/modules/gspca_main.ko");
								params.add("rmmod /system/lib/modules/gspca_zc3xx.ko");
								params.add("mount -o rw,remount /system");
								params.add("rm /system/lib/modules/gspca_main.ko");
								params.add("rm /system/lib/modules/gspca_zc3xx.ko");
								params.add("mount -o ro,remount /system");
								params.add("exit");
								ExecCmd ex = new ExecCmd(params);
								ex.run();
								final String msg;
								if (!ex.status) {
									// Log.d("microscope","Remove FAIL!");
									msg = "Remove driver fail!";
								} else {
									msg = "Remove driver OK";
								}
								Microscope.handler.post(new Runnable() {
									public void run() {
										Microscope.printStatusMsg(msg);
									}
								});
								finish();
							}
						})
				.setNegativeButton(android.R.string.no,
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int id) {
								dialog.cancel();
							}
						}).show();
	}

}
