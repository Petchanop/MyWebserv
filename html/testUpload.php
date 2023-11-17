<?php

// Get the uploaded file.
$file = $_FILES['file'];

// Check if the file was uploaded successfully.
if ($file['error'] == UPLOAD_ERR_OK) {
  // Create the directory "uploads/" if it does not exist.
  if (!is_dir('./uploads/')) {
    mkdir('./uploads/');
  }

  // Move the uploaded file to a permanent location.
  move_uploaded_file($file['tmp_name'], 'uploads/' . $file['name']);

  // Redirect the user to the success page.
  header('Location: http://localhost:8080/success.php');
} else {
  // Display an error message.
  echo 'Failed to upload file.';
}

?>