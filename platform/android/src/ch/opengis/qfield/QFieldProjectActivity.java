package ch.opengis.qfield;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Application;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.text.Html;
import android.text.TextUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.PopupMenu;
import android.widget.PopupMenu.OnMenuItemClickListener;
import android.widget.Toast;
import androidx.core.content.ContextCompat;
import androidx.core.content.FileProvider;
import androidx.core.view.MenuCompat;
import androidx.documentfile.provider.DocumentFile;
import ch.opengis.qfield.QFieldUtils;
import ch.opengis.qfield.R;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class QFieldProjectActivity
    extends Activity implements OnMenuItemClickListener {

    private static final String TAG = "QField Project Activity";
    private String path;
    private SharedPreferences sharedPreferences;
    SharedPreferences.Editor editor;
    private ListView list;
    private int currentPosition;

    ExecutorService executorService = Executors.newFixedThreadPool(4);

    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);

        sharedPreferences = getPreferences(Context.MODE_PRIVATE);
        editor = sharedPreferences.edit();
        setContentView(R.layout.list_projects);
        getActionBar().setBackgroundDrawable(
            new ColorDrawable(Color.parseColor("#80CC28")));
        getActionBar().setDisplayHomeAsUpEnabled(true);
        drawView();
    }

    @Override
    public boolean onNavigateUp() {
        finish();
        return true;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        if (!getIntent().hasExtra("path")) {
            getMenuInflater().inflate(R.menu.project_menu, menu);
            MenuCompat.setGroupDividerEnabled(menu, true);
            return true;
        }
        return false;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.import_dataset: {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION |
                                Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                intent.addFlags(Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
                intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
                intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);
                intent.setType("*/*");
                startActivityForResult(intent, R.id.import_dataset);
                return true;
            }
            case R.id.import_project_folder: {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION |
                                Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                intent.addFlags(Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
                intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
                startActivityForResult(intent, R.id.import_project_folder);
                return true;
            }
            case R.id.import_project_archive: {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION |
                                Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                intent.addFlags(Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
                intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
                intent.setType("application/zip");
                startActivityForResult(intent, R.id.import_project_archive);
                return true;
            }
            case R.id.usb_cable_help: {
                String url = "https://qfield.org/docs/";
                Intent i = new Intent(Intent.ACTION_VIEW);
                i.setData(Uri.parse(url));
                startActivity(i);
                return true;
            }
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    public boolean onMenuItemClick(MenuItem item) {
        final QFieldProjectListItem listItem =
            (QFieldProjectListItem)list.getAdapter().getItem(currentPosition);
        File file = listItem.getFile();
        switch (item.getItemId()) {
            // dataset-related actions
            case R.id.send_to: {
                DocumentFile documentFile = DocumentFile.fromFile(file);
                Context context = getApplication().getApplicationContext();
                Intent intent = new Intent(Intent.ACTION_SEND);
                intent.putExtra(Intent.EXTRA_STREAM,
                                FileProvider.getUriForFile(
                                    context,
                                    context.getPackageName() + ".fileprovider",
                                    file));
                intent.setType(documentFile.getType());
                startActivity(Intent.createChooser(intent, null));
                return true;
            }
            case R.id.remove_dataset: {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle(getString(R.string.delete_confirm_title));
                builder.setMessage(getString(R.string.delete_confirm_dataset));
                builder.setPositiveButton(
                    getString(R.string.delete_confirm),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            final QFieldProjectListItem listItem =
                                (QFieldProjectListItem)list.getAdapter()
                                    .getItem(currentPosition);
                            File file = listItem.getFile();
                            file.delete();
                            dialog.dismiss();
                            drawView();
                        }
                    });
                builder.setNegativeButton(
                    getString(R.string.delete_cancel),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            dialog.dismiss();
                        }
                    });
                AlertDialog dialog = builder.create();
                dialog.setCancelable(false);
                dialog.show();
                return true;
            }

            // folder-related actions
            case R.id.add_to_favorite: {
                addFileToFavoriteDirs(file);
                return true;
            }
            case R.id.remove_from_favorite: {
                removeFileFromFavoriteDirs(file);
                return true;
            }
            case R.id.send_compressed_to: {
                File temporaryFile =
                    new File(getCacheDir(), file.getName() + ".zip");
                QFieldUtils.zipFolder(file.getPath(), temporaryFile.getPath());

                DocumentFile documentFile =
                    DocumentFile.fromFile(temporaryFile);
                Context context = getApplication().getApplicationContext();
                Intent intent = new Intent(Intent.ACTION_SEND);
                intent.putExtra(Intent.EXTRA_STREAM,
                                FileProvider.getUriForFile(
                                    context,
                                    context.getPackageName() + ".fileprovider",
                                    temporaryFile));
                intent.setType(documentFile.getType());
                startActivity(Intent.createChooser(intent, null));
                return true;
            }
            case R.id.export_to_folder: {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
                intent.addCategory(Intent.CATEGORY_DEFAULT);
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
                startActivityForResult(intent, R.id.export_to_folder);
                return true;
            }
            case R.id.remove_folder: {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle(getString(R.string.delete_confirm_title));
                builder.setMessage(getString(R.string.delete_confirm_folder));
                builder.setPositiveButton(
                    getString(R.string.delete_confirm),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            final QFieldProjectListItem listItem =
                                (QFieldProjectListItem)list.getAdapter()
                                    .getItem(currentPosition);
                            QFieldUtils.deleteDirectory(listItem.getFile(),
                                                        true);
                            dialog.dismiss();
                            drawView();
                        }
                    });
                builder.setNegativeButton(
                    getString(R.string.delete_cancel),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            dialog.dismiss();
                        }
                    });
                AlertDialog dialog = builder.create();
                dialog.setCancelable(false);
                dialog.show();
                return true;
            }
            default:
                return true;
        }
    }

    private void drawView() {
        ArrayList<QFieldProjectListItem> values =
            new ArrayList<QFieldProjectListItem>();

        final boolean isRootView = !getIntent().hasExtra("path");
        if (isRootView) {
            File externalStorageDirectory = null;
            if (ContextCompat.checkSelfPermission(
                    QFieldProjectActivity.this,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE) ==
                    PackageManager.PERMISSION_GRANTED ||
                (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R &&
                 Environment.isExternalStorageManager())) {
                externalStorageDirectory =
                    Environment.getExternalStorageDirectory();
                Log.d(TAG,
                      "externalStorageDirectory: " + externalStorageDirectory);
                if (externalStorageDirectory != null) {
                    values.add(new QFieldProjectListItem(
                        externalStorageDirectory,
                        getString(R.string.primary_storage), R.drawable.tablet,
                        QFieldProjectListItem.TYPE_ROOT));
                }
            }

            File primaryExternalFilesDir = getExternalFilesDir(null);
            if (primaryExternalFilesDir != null) {
                values.add(new QFieldProjectListItem(
                    primaryExternalFilesDir,
                    getString(R.string.secondary_storage),
                    R.drawable.directory_qfield,
                    QFieldProjectListItem.TYPE_EXTERNAL_FILES));
            }

            File[] externalFilesDirs = getExternalFilesDirs(null);
            Log.d(TAG, "primaryExternalFilesDir: " +
                           primaryExternalFilesDir.getAbsolutePath());
            Log.d(TAG,
                  "externalFilesDirs: " + Arrays.toString(externalFilesDirs));
            for (File file : externalFilesDirs) {
                if (file != null) {
                    // Don't duplicate external files directory or storage
                    // path already added
                    if (file.getAbsolutePath().equals(
                            primaryExternalFilesDir.getAbsolutePath())) {
                        continue;
                    }
                    if (externalStorageDirectory != null) {
                        if (!file.getAbsolutePath().contains(
                                externalStorageDirectory.getAbsolutePath())) {
                            values.add(new QFieldProjectListItem(
                                file,
                                getString(R.string.secondary_storage_extra),
                                R.drawable.directory,
                                QFieldProjectListItem.TYPE_EXTERNAL_FILES));
                        }
                    } else {
                        values.add(new QFieldProjectListItem(
                            file, getString(R.string.secondary_storage_extra),
                            R.drawable.directory,
                            QFieldProjectListItem.TYPE_EXTERNAL_FILES));
                    }
                }
            }

            setTitle(getString(R.string.select_project));
            Collections.sort(values);

            String sampleProjects = new File(getFilesDir().toString() +
                                             "/share/qfield/sample_projects/")
                                        .getAbsolutePath();
            String importDatasetsDir = "";
            String importProjectsDir = "";
            if (externalFilesDirs.length > 0) {
                importDatasetsDir =
                    new File(externalFilesDirs[0].getAbsolutePath() +
                             "/Imported Datasets/")
                        .getAbsolutePath();
                importProjectsDir =
                    new File(externalFilesDirs[0].getAbsolutePath() +
                             "/Imported Projects/")
                        .getAbsolutePath();
            }

            String favoriteDirs =
                sharedPreferences.getString("FavoriteDirs", null);

            // The first time, add sample projects and import directories to
            // the favorites
            boolean favoriteDirsAdded =
                sharedPreferences.getBoolean("FavoriteDirsAdded", false);
            if (!favoriteDirsAdded) {
                favoriteDirs =
                    sampleProjects +
                    (importProjectsDir != "" ? "--;--" + importProjectsDir
                                             : "") +
                    (importDatasetsDir != "" ? "--;--" + importDatasetsDir
                                             : "");
                editor.putString("FavoriteDirs", favoriteDirs);
                editor.putBoolean("FavoriteDirsAdded", true);
                editor.commit();
            }
            if (favoriteDirs != null) {
                String[] favoriteDirsArray = favoriteDirs.split("--;--");
                values.add(new QFieldProjectListItem(
                    null, getString(R.string.favorite_directories), 0,
                    QFieldProjectListItem.TYPE_SEPARATOR));

                for (int i = favoriteDirsArray.length - 1; i >= 0; i--) {
                    File f = new File(favoriteDirsArray[i]);
                    if (f.exists()) {
                        String filePath = f.getAbsolutePath();
                        String favoriteName = f.getName();
                        if (filePath.equals(sampleProjects)) {
                            favoriteName =
                                getString(R.string.favorites_sample_projects);
                        } else if (filePath.equals(importDatasetsDir)) {
                            favoriteName =
                                getString(R.string.favorites_imported_datasets);
                        } else if (filePath.equals(importProjectsDir)) {
                            favoriteName =
                                getString(R.string.favorites_imported_projects);
                        }
                        values.add(new QFieldProjectListItem(
                            f, favoriteName, R.drawable.directory,
                            QFieldProjectListItem.TYPE_ITEM));
                    }
                }
            }
        } else {
            Log.d(TAG, "extra path: " + getIntent().getStringExtra("path"));
            File dir = new File(getIntent().getStringExtra("path"));
            setTitle(getIntent().getStringExtra("label"));
            getActionBar().setSubtitle(dir.getPath());

            if (!dir.canRead()) {
                setTitle(getTitle() + " (" + getString(R.string.inaccessible) +
                         ")");
            }
            File[] list = dir.listFiles();
            if (list != null) {
                for (File file : list) {
                    if (file.getName().startsWith(".")) {
                        continue;
                    } else if (file.getName().toLowerCase().endsWith(".qgs") ||
                               file.getName().toLowerCase().endsWith(".qgz") ||
                               file.getName().toLowerCase().endsWith(".gpkg") ||
                               file.getName().toLowerCase().endsWith(".shp") ||
                               file.getName().toLowerCase().endsWith(".kml") ||
                               file.getName().toLowerCase().endsWith(".kmz") ||
                               file.getName().toLowerCase().endsWith(
                                   ".geojson") ||
                               file.getName().toLowerCase().endsWith(".json") ||
                               file.getName().toLowerCase().endsWith(".gml") ||
                               file.getName().toLowerCase().endsWith(".mif") ||
                               file.getName().toLowerCase().endsWith(".fgb") ||
                               file.getName().toLowerCase().endsWith(".db") ||
                               file.getName().toLowerCase().endsWith(
                                   ".sqlite") ||
                               file.getName().toLowerCase().endsWith(".tif") ||
                               file.getName().toLowerCase().endsWith(".jpg") ||
                               file.getName().toLowerCase().endsWith(".png") ||
                               file.getName().toLowerCase().endsWith(".pdf") ||
                               file.getName().toLowerCase().endsWith(".gpx") ||
                               file.getName().toLowerCase().endsWith(".jp2") ||
                               file.getName().toLowerCase().endsWith(".webp") ||
                               file.getName().toLowerCase().endsWith(".vrt") ||
                               file.getName().toLowerCase().endsWith(".zip")) {
                        values.add(new QFieldProjectListItem(
                            file, file.getName(),
                            file.getName().toLowerCase().endsWith(".qgs") ||
                                    file.getName().toLowerCase().endsWith(
                                        ".qgz")
                                ? R.drawable.project
                                : R.drawable.dataset,
                            QFieldProjectListItem.TYPE_ITEM));
                    } else if (file.isDirectory()) {
                        values.add(new QFieldProjectListItem(
                            file, file.getName(), R.drawable.directory,
                            QFieldProjectListItem.TYPE_ITEM));
                    }
                }
            }
            Collections.sort(values);
        }

        // Put the data into the list
        list = (ListView)findViewById(R.id.list);
        QFieldProjectListAdapter adapter = new QFieldProjectListAdapter(
            this, values, new QFieldProjectListAdapter.MenuButtonListener() {
                @Override
                public void onClick(View view, int position) {
                    QFieldProjectActivity.this.onItemMenuClick(view, position);
                }
            });
        list.setAdapter(adapter);

        list.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view,
                                    int position, long id) {
                QFieldProjectActivity.this.onItemClick(position);
            }
        });

        list.setOnItemLongClickListener(new OnItemLongClickListener() {
            public boolean onItemLongClick(AdapterView<?> parent, View view,
                                           int position, long id) {
                return QFieldProjectActivity.this.onItemLongClick(position);
            }
        });
    }

    public void onRestart() {
        // The first opened activity
        if (!getIntent().hasExtra("path")) {
            drawView();
        }
        super.onRestart();
    }

    private void onItemMenuClick(View view, int position) {
        final QFieldProjectListItem item =
            (QFieldProjectListItem)list.getAdapter().getItem(position);
        File file = item.getFile();

        boolean isImportedDataset = false;
        boolean isImportedProjectRoot = false;
        File primaryExternalFilesDir = getExternalFilesDir(null);
        if (primaryExternalFilesDir != null) {
            isImportedDataset = file.getAbsolutePath().startsWith(
                new File(primaryExternalFilesDir.getAbsolutePath() +
                         "/Imported Datasets/")
                    .getAbsolutePath());
            if (file.getParentFile() != null) {
                isImportedProjectRoot =
                    file.getParentFile().getAbsolutePath().equals(
                        new File(primaryExternalFilesDir.getAbsolutePath() +
                                 "/Imported Projects/")
                            .getAbsolutePath());
            }
        }

        PopupMenu popupMenu = new PopupMenu(QFieldProjectActivity.this, view);
        popupMenu.setOnMenuItemClickListener(QFieldProjectActivity.this);

        if (!file.isDirectory()) {
            popupMenu.inflate(R.menu.project_item_menu);
            if (!isImportedDataset) {
                popupMenu.getMenu()
                    .findItem(R.id.remove_dataset)
                    .setVisible(false);
            }
        } else {
            String favoriteDirs =
                sharedPreferences.getString("FavoriteDirs", null);
            ArrayList<String> favoriteDirsArray = new ArrayList<String>();
            if (favoriteDirs != null) {
                favoriteDirsArray = new ArrayList<String>(
                    Arrays.asList(favoriteDirs.split("--;--")));
            }
            boolean isFavorite = false;
            for (String favoriteDir : favoriteDirsArray) {
                if (favoriteDir.equals(file.getPath())) {
                    isFavorite = true;
                    break;
                }
            }

            popupMenu.inflate(R.menu.project_folder_menu);
            if (!isFavorite) {
                popupMenu.getMenu()
                    .findItem(R.id.remove_from_favorite)
                    .setVisible(false);
            } else {
                popupMenu.getMenu()
                    .findItem(R.id.add_to_favorite)
                    .setVisible(false);
            }
            if (!isImportedProjectRoot) {
                popupMenu.getMenu()
                    .findItem(R.id.remove_folder)
                    .setVisible(false);
            }
        }

        currentPosition = position;
        popupMenu.show();
    }

    private void onItemClick(int position) {
        final QFieldProjectListItem item =
            (QFieldProjectListItem)list.getAdapter().getItem(position);
        if (item.getType() == QFieldProjectListItem.TYPE_SEPARATOR) {
            return;
        }
        // Show an information notice if it's the first time the external
        // files directory is used
        boolean showExternalFilesNotice =
            sharedPreferences.getBoolean("ShowExternalFilesNotice", true);
        if (item.getType() == QFieldProjectListItem.TYPE_EXTERNAL_FILES &&
            showExternalFilesNotice) {
            AlertDialog alertDialog = new AlertDialog.Builder(this).create();
            alertDialog.setTitle(getString(R.string.external_files_title));
            alertDialog.setMessage(
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.N
                    ? Html.fromHtml(getString(R.string.external_files_message),
                                    Html.FROM_HTML_MODE_LEGACY)
                    : Html.fromHtml(
                          getString(R.string.external_files_message)));
            alertDialog.setButton(AlertDialog.BUTTON_POSITIVE,
                                  getString(R.string.external_files_ok),
                                  new DialogInterface.OnClickListener() {
                                      public void onClick(
                                          DialogInterface dialog, int which) {
                                          dialog.dismiss();
                                          startItemClickActivity(item);
                                      }
                                  });
            alertDialog.show();
            editor.putBoolean("ShowExternalFilesNotice", false);
            editor.commit();
        } else {
            startItemClickActivity(item);
        }
    }

    private void startItemClickActivity(QFieldProjectListItem item) {
        File file = item.getFile();
        if (file.isDirectory()) {
            Intent intent = new Intent(this, QFieldProjectActivity.class);
            intent.putExtra("path", file.getPath());
            intent.putExtra("label", item.getText());
            startActivityForResult(intent, 123);
        } else {
            Intent data = new Intent();

            Uri uri = Uri.fromFile(file);
            data.setData(uri);
            setResult(Activity.RESULT_OK, data);

            String lastUsedProjects =
                sharedPreferences.getString("LastUsedProjects", null);
            ArrayList<String> lastUsedProjectsArray = new ArrayList<String>();
            if (lastUsedProjects != null) {
                lastUsedProjectsArray = new ArrayList<String>(
                    Arrays.asList(lastUsedProjects.split("--;--")));
            }
            // If the element is already present, delete it. It will be
            // added again in the last position
            lastUsedProjectsArray.remove(file.getPath());
            if (lastUsedProjectsArray.size() >= 5) {
                lastUsedProjectsArray.remove(0);
            }
            // Add the project path to the array
            lastUsedProjectsArray.add(file.getPath());

            // Write the recent projects into the shared preferences
            editor.putString("LastUsedProjects",
                             TextUtils.join("--;--", lastUsedProjectsArray));
            editor.commit();

            finish();
        }
    }

    private boolean onItemLongClick(int position) {
        QFieldProjectListItem item =
            (QFieldProjectListItem)list.getAdapter().getItem(position);
        if (item.getType() != QFieldProjectListItem.TYPE_ITEM) {
            return true;
        }
        File file = item.getFile();
        if (!file.isDirectory()) {
            return true;
        }

        // First activity
        if (!getIntent().hasExtra("path")) {
            removeFileFromFavoriteDirs(file);
        } else {
            addFileToFavoriteDirs(file);
        }

        return true;
    }

    void addFileToFavoriteDirs(File file) {
        String favoriteDirs = sharedPreferences.getString("FavoriteDirs", null);
        ArrayList<String> favoriteDirsArray = new ArrayList<String>();
        if (favoriteDirs != null) {
            favoriteDirsArray = new ArrayList<String>(
                Arrays.asList(favoriteDirs.split("--;--")));
        }

        // If the element is already present, delete it. It will be added
        // again in the last position
        favoriteDirsArray.remove(file.getPath());

        // Write the recent projects into the shared preferences
        favoriteDirsArray.add(file.getPath());
        editor.putString("FavoriteDirs",
                         TextUtils.join("--;--", favoriteDirsArray));
        editor.commit();

        Toast
            .makeText(this,
                      file.getName() + " " +
                          getString(R.string.added_to_favorites),
                      Toast.LENGTH_LONG)
            .show();
    }

    void removeFileFromFavoriteDirs(File file) {

        String favoriteDirs = sharedPreferences.getString("FavoriteDirs", null);
        ArrayList<String> favoriteDirsArray = new ArrayList<String>();
        if (favoriteDirs != null) {
            favoriteDirsArray = new ArrayList<String>(
                Arrays.asList(favoriteDirs.split("--;--")));
        }

        // If the element is already present, delete it. It will be added
        // again in the last position
        favoriteDirsArray.remove(file.getPath());

        favoriteDirs = TextUtils.join("--;--", favoriteDirsArray);
        if (favoriteDirs == "") {
            favoriteDirs = null;
        }

        editor.putString("FavoriteDirs", favoriteDirs);
        editor.commit();
        if (!getIntent().hasExtra("path")) {
            // Root view, redraw
            drawView();
        }

        Toast
            .makeText(this,
                      file.getName() + " " +
                          getString(R.string.removed_from_favorites),
                      Toast.LENGTH_LONG)
            .show();
    }

    void importDatasets(Uri[] datasetUris) {
        File externalFilesDir = getExternalFilesDir(null);
        if (externalFilesDir == null || datasetUris.length == 0) {
            return;
        }

        ProgressDialog progressDialog = ProgressDialog.show(
            this, "", "Please wait while QField is importing the project",
            true);
        progressDialog.setCancelable(false);

        String importDatasetPath =
            externalFilesDir.getAbsolutePath() + "/Imported Datasets/";
        new File(importDatasetPath).mkdir();

        Context context = getApplication().getApplicationContext();
        ContentResolver resolver = getContentResolver();

        executorService.execute(new Runnable() {
            @Override
            public void run() {
                boolean imported = false;
                for (Uri datasetUri : datasetUris) {
                    DocumentFile documentFile =
                        DocumentFile.fromSingleUri(context, datasetUri);
                    String importFilePath =
                        importDatasetPath + documentFile.getName();
                    try {
                        InputStream input =
                            resolver.openInputStream(datasetUri);
                        imported = QFieldUtils.inputStreamToFile(
                            input, importFilePath, documentFile.length());
                    } catch (Exception e) {
                        e.printStackTrace();
                        imported = false;
                    }
                    if (!imported) {
                        break;
                    }
                }

                progressDialog.dismiss();
                if (!imported) {
                    AlertDialog alertDialog =
                        new AlertDialog.Builder(QFieldProjectActivity.this)
                            .create();
                    alertDialog.setTitle(getString(R.string.import_error));
                    alertDialog.setMessage(
                        getString(R.string.import_dataset_error));
                    if (!isFinishing()) {
                        alertDialog.show();
                    }
                } else {
                    Intent intent = new Intent(QFieldProjectActivity.this,
                                               QFieldProjectActivity.class);
                    intent.putExtra("path", importDatasetPath);
                    intent.putExtra(
                        "label",
                        getString(R.string.favorites_imported_datasets));
                    startActivityForResult(intent, 123);
                }
            }
        });
    }

    void importProjectFolder(Uri folderUri) {
        File externalFilesDir = getExternalFilesDir(null);
        if (externalFilesDir == null) {
            return;
        }

        ProgressDialog progressDialog = ProgressDialog.show(
            this, "", "Please wait while QField is importing the project",
            true);
        progressDialog.setCancelable(false);

        String importProjectPath =
            externalFilesDir.getAbsolutePath() + "/Imported Projects/";
        new File(importProjectPath).mkdir();

        Context context = getApplication().getApplicationContext();
        ContentResolver resolver = getContentResolver();

        executorService.execute(new Runnable() {
            @Override
            public void run() {
                DocumentFile directory =
                    DocumentFile.fromTreeUri(context, folderUri);
                String importPath =
                    importProjectPath + directory.getName() + "/";
                new File(importPath).mkdir();
                boolean imported = QFieldUtils.documentFileToFolder(
                    directory, importPath, resolver);

                progressDialog.dismiss();
                if (imported) {
                    Intent intent = new Intent(QFieldProjectActivity.this,
                                               QFieldProjectActivity.class);
                    intent.putExtra("path", importProjectPath);
                    intent.putExtra(
                        "label",
                        getString(R.string.favorites_imported_projects));
                    startActivityForResult(intent, 123);
                } else {
                    AlertDialog alertDialog =
                        new AlertDialog.Builder(QFieldProjectActivity.this)
                            .create();
                    alertDialog.setTitle(getString(R.string.import_error));
                    alertDialog.setMessage(
                        getString(R.string.import_project_folder_error));
                    if (!isFinishing()) {
                        alertDialog.show();
                    }
                }
            }
        });
    }

    void importProjectArchive(Uri archiveUri) {
        File externalFilesDir = getExternalFilesDir(null);
        if (externalFilesDir == null) {
            return;
        }

        ProgressDialog progressDialog = ProgressDialog.show(
            this, "", "Please wait while QField is importing the project",
            true);
        progressDialog.setCancelable(false);

        String importProjectPath =
            externalFilesDir.getAbsolutePath() + "/Imported Projects/";
        new File(importProjectPath).mkdir();

        Context context = getApplication().getApplicationContext();
        ContentResolver resolver = getContentResolver();

        executorService.execute(new Runnable() {
            @Override
            public void run() {
                DocumentFile documentFile =
                    DocumentFile.fromSingleUri(context, archiveUri);

                String projectName = "";
                try {
                    InputStream input = resolver.openInputStream(archiveUri);
                    projectName = QFieldUtils.getArchiveProjectName(input);
                } catch (Exception e) {
                    e.printStackTrace();
                }

                if (projectName != "") {
                    String importPath =
                        importProjectPath +
                        documentFile.getName().substring(
                            0, documentFile.getName().lastIndexOf(".")) +
                        "/";
                    new File(importPath).mkdir();
                    boolean imported = false;
                    try {
                        InputStream input =
                            resolver.openInputStream(archiveUri);
                        imported =
                            QFieldUtils.inputStreamToFolder(input, importPath);
                    } catch (Exception e) {
                        e.printStackTrace();
                        AlertDialog alertDialog =
                            new AlertDialog.Builder(QFieldProjectActivity.this)
                                .create();
                        alertDialog.setTitle(getString(R.string.import_error));
                        alertDialog.setMessage(
                            getString(R.string.import_project_archive_error));
                        if (!isFinishing()) {
                            alertDialog.show();
                        }
                    }

                    progressDialog.dismiss();
                    if (imported) {
                        Intent intent = new Intent(QFieldProjectActivity.this,
                                                   QFieldProjectActivity.class);
                        intent.putExtra("path", importProjectPath);
                        intent.putExtra(
                            "label",
                            getString(R.string.favorites_imported_projects));
                        startActivityForResult(intent, 123);
                    }
                } else {
                    progressDialog.dismiss();
                }
            }
        });
    }

    protected void onActivityResult(int requestCode, int resultCode,
                                    Intent data) {
        Log.d(TAG, "activity result requestCode: " + requestCode);
        Log.d(TAG, "activity result resultCode: " + resultCode);

        if (requestCode == R.id.import_dataset &&
            resultCode == Activity.RESULT_OK) {
            Log.d(TAG, "handling import dataset(s)");
            File externalFilesDir = getExternalFilesDir(null);
            if (externalFilesDir == null || data == null) {
                return;
            }

            String importDatasetPath =
                externalFilesDir.getAbsolutePath() + "/Imported Datasets/";

            Context context = getApplication().getApplicationContext();
            ContentResolver resolver = getContentResolver();

            Uri[] datasetUris;
            if (data.getClipData() != null) {
                datasetUris = new Uri[data.getClipData().getItemCount()];
                for (int i = 0; i < data.getClipData().getItemCount(); i++) {
                    datasetUris[i] = data.getClipData().getItemAt(i).getUri();
                }
            } else {
                datasetUris = new Uri[1];
                datasetUris[0] = data.getData();
            }

            boolean hasExists = false;
            for (Uri datasetUri : datasetUris) {
                DocumentFile documentFile =
                    DocumentFile.fromSingleUri(context, datasetUri);
                File importFilePath =
                    new File(importDatasetPath + documentFile.getName());
                if (importFilePath.exists()) {
                    hasExists = true;
                    break;
                }
            }

            if (hasExists) {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle(getString(R.string.import_overwrite_title));
                builder.setMessage(
                    datasetUris.length > 1
                        ? getString(R.string.import_overwrite_dataset_multiple)
                        : getString(R.string.import_overwrite_dataset_single));
                builder.setPositiveButton(
                    getString(R.string.import_overwrite_confirm),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            importDatasets(datasetUris);
                            dialog.dismiss();
                        }
                    });
                builder.setNegativeButton(
                    getString(R.string.import_overwrite_cancel),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            dialog.dismiss();
                        }
                    });
                AlertDialog dialog = builder.create();
                dialog.setCancelable(false);
                dialog.show();
            } else {
                importDatasets(datasetUris);
            }
        } else if (requestCode == R.id.import_project_folder &&
                   resultCode == Activity.RESULT_OK) {
            Log.d(TAG, "handling import project folder");
            File externalFilesDir = getExternalFilesDir(null);
            if (externalFilesDir == null || data == null) {
                return;
            }

            Uri uri = data.getData();
            Context context = getApplication().getApplicationContext();
            DocumentFile directory = DocumentFile.fromTreeUri(context, uri);
            File importPath =
                new File(externalFilesDir.getAbsolutePath() +
                         "/Imported Projects/" + directory.getName() + "/");
            if (importPath.exists()) {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle(getString(R.string.import_overwrite_title));
                builder.setMessage(getString(R.string.import_overwrite_folder));
                builder.setPositiveButton(
                    getString(R.string.import_overwrite_confirm),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            importProjectFolder(uri);
                            dialog.dismiss();
                        }
                    });
                builder.setNegativeButton(
                    getString(R.string.import_overwrite_cancel),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            dialog.dismiss();
                        }
                    });
                AlertDialog dialog = builder.create();
                dialog.setCancelable(false);
                dialog.show();
            } else {
                importProjectFolder(uri);
            }
        } else if (requestCode == R.id.import_project_archive &&
                   resultCode == Activity.RESULT_OK) {
            Log.d(TAG, "handling import project archive");
            File externalFilesDir = getExternalFilesDir(null);
            if (externalFilesDir == null || data == null) {
                return;
            }

            String importProjectPath =
                externalFilesDir.getAbsolutePath() + "/Imported Projects/";
            new File(importProjectPath).mkdir();

            Uri uri = data.getData();
            Context context = getApplication().getApplicationContext();
            ContentResolver resolver = getContentResolver();

            DocumentFile documentFile =
                DocumentFile.fromSingleUri(context, uri);
            File importPath = new File(
                externalFilesDir.getAbsolutePath() + "/Imported Projects/" +
                documentFile.getName().substring(
                    0, documentFile.getName().lastIndexOf(".")) +
                "/");
            if (importPath.exists()) {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle(getString(R.string.import_overwrite_title));
                builder.setMessage(getString(R.string.import_overwrite_folder));
                builder.setPositiveButton(
                    getString(R.string.import_overwrite_confirm),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            importProjectArchive(uri);
                            dialog.dismiss();
                        }
                    });
                builder.setNegativeButton(
                    getString(R.string.import_overwrite_cancel),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            dialog.dismiss();
                        }
                    });
                AlertDialog dialog = builder.create();
                dialog.setCancelable(false);
                dialog.show();
            } else {
                importProjectArchive(uri);
            }
        } else if (requestCode == R.id.export_to_folder &&
                   resultCode == Activity.RESULT_OK) {
            Log.d(TAG, "handling export to folder");

            final QFieldProjectListItem listItem =
                (QFieldProjectListItem)list.getAdapter().getItem(
                    currentPosition);
            File file = listItem.getFile();
            Uri uri = data.getData();
            Context context = getApplication().getApplicationContext();
            ContentResolver resolver = getContentResolver();

            executorService.execute(new Runnable() {
                @Override
                public void run() {
                    resolver.takePersistableUriPermission(
                        uri, Intent.FLAG_GRANT_READ_URI_PERMISSION |
                                 Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                    DocumentFile directory =
                        DocumentFile.fromTreeUri(context, uri);

                    boolean exported = exported =
                        QFieldUtils.fileToDocumentFile(file, directory,
                                                       resolver);

                    if (!exported) {
                        AlertDialog alertDialog =
                            new AlertDialog.Builder(QFieldProjectActivity.this)
                                .create();
                        alertDialog.setTitle(getString(R.string.export_error));
                        alertDialog.setMessage(
                            getString(R.string.export_to_folder_error));
                        if (!isFinishing()) {
                            alertDialog.show();
                        }
                    }
                }
            });
        } else if (resultCode == Activity.RESULT_OK) {
            // Close recursively the activity stack
            if (getParent() == null) {
                setResult(Activity.RESULT_OK, data);
            } else {
                getParent().setResult(Activity.RESULT_OK, data);
            }
            finish();
        }
    }
}
