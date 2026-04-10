<?php
/**
 * @file
 * Admin form for adding extra sample content.
 */

/**
 * Form callback: Sample Animal Content configuration page.
 */
function sample_animal_content_admin_form($form, &$form_state) {
  $config = config('sample_animal_content.settings');
  $extras = $config->get('extras_added');

  $form['#tree'] = TRUE;

  // Extra animals section.
  $form['extra_animals'] = array(
    '#type' => 'fieldset',
    '#title' => t('Additional Animals'),
    '#description' => t('Add 10 more animal nodes with new taxonomy classes (Amphibia, Chondrichthyes, Cephalopoda).'),
  );
  if (!empty($extras['extra_animals'])) {
    $form['extra_animals']['status'] = array(
      '#type' => 'markup',
      '#markup' => '<p>' . t('Additional animals have been added.') . '</p>',
    );
  }
  else {
    $form['extra_animals']['add_extra_animals'] = array(
      '#type' => 'submit',
      '#value' => t('Add More Animals'),
      '#submit' => array('sample_animal_content_add_extra_animals_submit'),
    );
  }

  // Comments section.
  $form['comments'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sample Comments'),
    '#description' => t('Add threaded sample comments to the animal nodes.'),
  );
  if (!empty($extras['comments'])) {
    $tracked_cids = $config->get('tracked_cids');
    $form['comments']['status'] = array(
      '#type' => 'markup',
      '#markup' => '<p>' . t('Sample comments have been added (@count comments tracked).', array('@count' => count($tracked_cids))) . '</p>',
    );
  }
  else {
    $form['comments']['add_comments'] = array(
      '#type' => 'submit',
      '#value' => t('Add Sample Comments'),
      '#submit' => array('sample_animal_content_add_comments_submit'),
    );
  }

  // Habitats / node reference section.
  $form['habitats'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sample Habitats (Node References)'),
    '#description' => t('Create a Habitat content type with a node reference field to animals, plus sample habitat nodes.'),
  );
  if (!empty($extras['habitats'])) {
    $form['habitats']['status'] = array(
      '#type' => 'markup',
      '#markup' => '<p>' . t('Sample habitats have been added.') . '</p>',
    );
  }
  else {
    $form['habitats']['add_habitats'] = array(
      '#type' => 'submit',
      '#value' => t('Add Sample Habitats'),
      '#submit' => array('sample_animal_content_add_habitats_submit'),
    );
  }

  // Placeholder for future removal feature.
  $form['removal'] = array(
    '#type' => 'fieldset',
    '#title' => t('Remove Sample Content'),
    '#description' => t('Safely remove sample content created by this module. Only content that has not been modified will be removed.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['removal']['info'] = array(
    '#type' => 'markup',
    '#markup' => '<p><em>' . t('This feature is not yet available.') . '</em></p>',
  );

  return $form;
}

/**
 * Submit handler: Add 10 more animal nodes.
 */
function sample_animal_content_add_extra_animals_submit($form, &$form_state) {
  $config = config('sample_animal_content.settings');
  $module_path = backdrop_get_path('module', 'sample_animal_content');
  $image_directory = 'public://sample_animal_images';
  file_prepare_directory($image_directory, FILE_CREATE_DIRECTORY);

  $animals = sample_animal_content_extra_list();
  $tracked_nids = $config->get('tracked_nids');
  $tracked_tids = $config->get('tracked_tids');
  $new_nids = array();

  // Create any new taxonomy terms that don't exist yet.
  foreach (array_unique(array_column($animals, 'class')) as $class_name) {
    $existing = taxonomy_term_load_multiple_by_name($class_name, 'animal_class');
    if (empty($existing)) {
      $term = entity_create('taxonomy_term', array(
        'name' => $class_name,
        'vocabulary' => 'animal_class',
      ));
      taxonomy_term_save($term);
      $tracked_tids[] = $term->tid;
    }
  }

  // Create the animal nodes.
  foreach ($animals as $name => $animal) {
    $node = new Node();
    $node->type = 'animal';
    $node->title = $animal['title'];
    node_object_prepare($node);
    $node->language = LANGUAGE_NONE;
    $node->uid = 1;

    // Class field.
    $classes = taxonomy_term_load_multiple_by_name($animal['class'], 'animal_class');
    $class = array_shift($classes);
    if ($class) {
      $node->field_animal_class[$node->language][0]['tid'] = $class->tid;
    }

    // Image field.
    if (!isset($animal['image']) || $animal['image'] !== FALSE) {
      $filename = empty($animal['image']) ? $name . '.jpg' : $animal['image'];
      $filepath = $module_path . '/images/' . $filename;
      if (file_exists($filepath)) {
        $data = file_get_contents($filepath);
        $image = file_save_data($data, $image_directory . '/' . $filename, FILE_EXISTS_REPLACE);
        $image->uid = 1;
        $node->field_image[$node->language][] = (array) $image;
      }
    }

    // Body field.
    if (!isset($animal['text']) || $animal['text'] !== FALSE) {
      $filename = empty($animal['text']) ? $name . '.txt' : $animal['text'];
      $filepath = $module_path . '/animals_txt/' . $filename;
      if (file_exists($filepath)) {
        $text = file_get_contents($filepath);
        $node->body[$node->language][] = array(
          'value' => $text,
          'format' => 'filtered_html',
        );
      }
    }

    // Link field.
    if (!empty($animal['link'])) {
      $node->field_animal_link[$node->language][0]['url'] = $animal['link'];
    }

    $node = node_submit($node);
    node_save($node);
    $new_nids[] = $node->nid;
  }

  // Update tracking.
  $config->set('tracked_nids', array_merge($tracked_nids, $new_nids));
  $config->set('tracked_tids', $tracked_tids);
  $config->set('extras_added.extra_animals', TRUE);
  $config->save();

  backdrop_set_message(t('Added @count additional animal nodes.', array('@count' => count($new_nids))));
}

/**
 * Submit handler: Add sample comments to animal nodes.
 */
function sample_animal_content_add_comments_submit($form, &$form_state) {
  // Ensure the comment module is enabled.
  if (!module_exists('comment')) {
    module_enable(array('comment'));
    backdrop_set_message(t('Enabled the Comment module.'));
  }

  // Enable comments on the animal content type.
  $animal_type = node_type_get_type('animal');
  if ($animal_type) {
    $animal_type->settings['comment_enabled'] = TRUE;
    $animal_type->settings['comment_default'] = COMMENT_NODE_OPEN;
    $animal_type->settings['comment_mode'] = COMMENT_MODE_THREADED;
    node_type_save($animal_type);
  }

  $config = config('sample_animal_content.settings');
  $tracked_nids = $config->get('tracked_nids');

  // Open comments on all existing animal nodes.
  if (!empty($tracked_nids)) {
    $nodes = node_load_multiple($tracked_nids);
    foreach ($nodes as $node) {
      $node->comment = COMMENT_NODE_OPEN;
      $node->save();
    }
  }

  if (empty($tracked_nids)) {
    backdrop_set_message(t('No tracked animal nodes found. Sample content may not have been installed properly.'), 'error');
    return;
  }

  // Load animal nodes.
  $nodes = node_load_multiple($tracked_nids);
  if (empty($nodes)) {
    backdrop_set_message(t('Could not load animal nodes. They may have been deleted.'), 'error');
    return;
  }

  $tracked_cids = array();
  $comments = sample_animal_content_comment_data();

  foreach ($nodes as $node) {
    $animal_key = strtolower(str_replace(' ', '_', $node->title));
    // Try to find comments for this node by matching against the keys.
    $node_comments = array();
    foreach ($comments as $key => $data) {
      if (isset($data['node_match']) && $data['node_match'] === $animal_key) {
        $node_comments = $data['threads'];
        break;
      }
    }

    // If no specific comments for this animal, use generic ones.
    if (empty($node_comments) && isset($comments['_generic'])) {
      $node_comments = $comments['_generic']['threads'];
    }

    if (empty($node_comments)) {
      continue;
    }

    foreach ($node_comments as $thread) {
      // Create top-level comment.
      $comment = entity_create('comment', array(
        'nid' => $node->nid,
        'pid' => 0,
        'uid' => 0,
        'name' => $thread['name'],
        'status' => COMMENT_PUBLISHED,
        'created' => REQUEST_TIME - $thread['age_offset'],
        'changed' => REQUEST_TIME - $thread['age_offset'],
      ));
      $comment->comment_body[LANGUAGE_NONE][0] = array(
        'value' => $thread['body'],
        'format' => 'filtered_html',
      );
      comment_save($comment);
      $tracked_cids[] = $comment->cid;

      // Create replies.
      if (!empty($thread['replies'])) {
        foreach ($thread['replies'] as $reply) {
          $reply_comment = entity_create('comment', array(
            'nid' => $node->nid,
            'pid' => $comment->cid,
            'uid' => 0,
            'name' => $reply['name'],
            'status' => COMMENT_PUBLISHED,
            'created' => REQUEST_TIME - $reply['age_offset'],
            'changed' => REQUEST_TIME - $reply['age_offset'],
          ));
          $reply_comment->comment_body[LANGUAGE_NONE][0] = array(
            'value' => $reply['body'],
            'format' => 'filtered_html',
          );
          comment_save($reply_comment);
          $tracked_cids[] = $reply_comment->cid;

          // Nested reply (third level).
          if (!empty($reply['replies'])) {
            foreach ($reply['replies'] as $nested) {
              $nested_comment = entity_create('comment', array(
                'nid' => $node->nid,
                'pid' => $reply_comment->cid,
                'uid' => 0,
                'name' => $nested['name'],
                'status' => COMMENT_PUBLISHED,
                'created' => REQUEST_TIME - $nested['age_offset'],
                'changed' => REQUEST_TIME - $nested['age_offset'],
              ));
              $nested_comment->comment_body[LANGUAGE_NONE][0] = array(
                'value' => $nested['body'],
                'format' => 'filtered_html',
              );
              comment_save($nested_comment);
              $tracked_cids[] = $nested_comment->cid;
            }
          }
        }
      }
    }
  }

  // Save tracking data.
  $config->set('tracked_cids', $tracked_cids);
  $config->set('extras_added.comments', TRUE);
  $config->save();

  backdrop_set_message(t('Added @count sample comments to animal nodes.', array('@count' => count($tracked_cids))));
}

/**
 * Submit handler: Add sample habitats with node references.
 */
function sample_animal_content_add_habitats_submit($form, &$form_state) {
  // Ensure the entityreference module is enabled.
  if (!module_exists('entityreference')) {
    module_enable(array('entityreference'));
    backdrop_set_message(t('Enabled the Entity Reference module.'));
  }

  $config = config('sample_animal_content.settings');
  $tracked_nids = $config->get('tracked_nids');

  if (empty($tracked_nids)) {
    backdrop_set_message(t('No tracked animal nodes found. Sample content may not have been installed properly.'), 'error');
    return;
  }

  $t = get_t();
  $tracked_habitat_nids = array();

  // Create the Habitat content type.
  $type = (object) array(
    'type' => 'habitat',
    'name' => $t('Habitat'),
    'description' => $t('A natural habitat where animals live.'),
    'settings' => array(
      'scheduling_enabled' => FALSE,
      'sticky_enabled' => FALSE,
      'promote_default' => FALSE,
      'node_submitted' => FALSE,
      'node_user_picture' => FALSE,
    ),
    'is_new' => TRUE,
  );
  node_type_save($type);

  // Body field for habitat.
  $instance = array(
    'field_name' => 'body',
    'label' => $t('Description'),
    'entity_type' => 'node',
    'bundle' => 'habitat',
    'widget' => array(
      'weight' => -3,
    ),
  );
  field_create_instance($instance);

  // Create a taxonomy vocabulary for habitat type with hierarchy.
  $vocab_exists = taxonomy_vocabulary_machine_name_load('habitat_type');
  if (!$vocab_exists) {
    $vocabulary = new TaxonomyVocabulary(array(
      'name' => $t('Habitat Type'),
      'machine_name' => 'habitat_type',
      'hierarchy' => 1,
    ));
    taxonomy_vocabulary_save($vocabulary);
  }

  // Create hierarchical terms.
  $tracked_habitat_tids = array();
  $habitat_terms = sample_animal_content_habitat_terms();
  $term_map = array();

  // First pass: parent terms.
  foreach ($habitat_terms as $key => $data) {
    if (empty($data['parent'])) {
      $term = entity_create('taxonomy_term', array(
        'name' => $data['name'],
        'vocabulary' => 'habitat_type',
        'parent' => array(0),
      ));
      taxonomy_term_save($term);
      $term_map[$key] = $term->tid;
      $tracked_habitat_tids[] = $term->tid;
    }
  }

  // Second pass: child terms.
  foreach ($habitat_terms as $key => $data) {
    if (!empty($data['parent']) && isset($term_map[$data['parent']])) {
      $term = entity_create('taxonomy_term', array(
        'name' => $data['name'],
        'vocabulary' => 'habitat_type',
        'parent' => array($term_map[$data['parent']]),
      ));
      taxonomy_term_save($term);
      $term_map[$key] = $term->tid;
      $tracked_habitat_tids[] = $term->tid;
    }
  }

  // Habitat type field on habitat content type.
  if (!field_info_field('field_habitat_type')) {
    $field = array(
      'field_name' => 'field_habitat_type',
      'type' => 'taxonomy_term_reference',
      'cardinality' => 1,
      'settings' => array(
        'allowed_values' => array(
          array(
            'vocabulary' => 'habitat_type',
            'parent' => '0',
          ),
        ),
      ),
    );
    field_create_field($field);
  }
  $instance = array(
    'field_name' => 'field_habitat_type',
    'label' => $t('Habitat Type'),
    'entity_type' => 'node',
    'bundle' => 'habitat',
    'widget' => array(
      'type' => 'options_select',
      'weight' => -4,
    ),
    'display' => array(
      'default' => array(
        'label' => 'inline',
      ),
    ),
  );
  field_create_instance($instance);

  // Create an entity reference field on the animal content type pointing to
  // habitat nodes, so each animal shows its habitat(s).
  if (!field_info_field('field_animal_habitat')) {
    $field = array(
      'field_name' => 'field_animal_habitat',
      'type' => 'entityreference',
      'cardinality' => FIELD_CARDINALITY_UNLIMITED,
      'settings' => array(
        'handler' => 'base',
        'target_type' => 'node',
        'handler_settings' => array(
          'target_bundles' => array('habitat' => 'habitat'),
        ),
      ),
    );
    field_create_field($field);
  }
  if (!field_info_instance('node', 'field_animal_habitat', 'animal')) {
    $instance = array(
      'field_name' => 'field_animal_habitat',
      'label' => $t('Habitat'),
      'entity_type' => 'node',
      'bundle' => 'animal',
      'description' => $t('The habitat(s) where this animal lives.'),
      'widget' => array(
        'type' => 'entityreference_autocomplete',
        'weight' => 5,
      ),
      'display' => array(
        'default' => array(
          'type' => 'entityreference_label',
          'label' => 'inline',
          'settings' => array(
            'link' => TRUE,
          ),
          'weight' => 5,
        ),
        'teaser' => array(
          'type' => 'entityreference_label',
          'label' => 'inline',
          'settings' => array(
            'link' => TRUE,
          ),
          'weight' => 5,
        ),
      ),
    );
    field_create_instance($instance);
  }

  cache_clear_all();

  // Create sample habitat nodes and build a map of title to NID.
  $habitats = sample_animal_content_habitat_data();
  $habitat_by_title = array();
  foreach ($habitats as $habitat_data) {
    $node = new Node();
    $node->type = 'habitat';
    $node->title = $habitat_data['title'];
    node_object_prepare($node);
    $node->language = LANGUAGE_NONE;
    $node->uid = 1;

    $node->body[LANGUAGE_NONE][0] = array(
      'value' => $habitat_data['body'],
      'format' => 'filtered_html',
    );

    // Set habitat type term.
    if (isset($habitat_data['type_key']) && isset($term_map[$habitat_data['type_key']])) {
      $node->field_habitat_type[LANGUAGE_NONE][0]['tid'] = $term_map[$habitat_data['type_key']];
    }

    $node = node_submit($node);
    node_save($node);
    $tracked_habitat_nids[] = $node->nid;
    $habitat_by_title[$habitat_data['title']] = $node->nid;
  }

  // Build a reverse map: animal title => array of habitat NIDs.
  $animal_habitats = array();
  foreach ($habitats as $habitat_data) {
    if (!empty($habitat_data['animals']) && isset($habitat_by_title[$habitat_data['title']])) {
      $habitat_nid = $habitat_by_title[$habitat_data['title']];
      foreach ($habitat_data['animals'] as $animal_title) {
        $animal_habitats[$animal_title][] = $habitat_nid;
      }
    }
  }

  // Update existing animal nodes with their habitat references.
  $animal_nodes = node_load_multiple($tracked_nids);
  foreach ($animal_nodes as $animal_node) {
    if (isset($animal_habitats[$animal_node->title])) {
      $delta = 0;
      foreach ($animal_habitats[$animal_node->title] as $habitat_nid) {
        $animal_node->field_animal_habitat[LANGUAGE_NONE][$delta]['target_id'] = $habitat_nid;
        $delta++;
      }
      $animal_node->save();
    }
  }

  // Update tracking.
  $existing_nids = $config->get('tracked_nids');
  $config->set('tracked_nids', array_merge($existing_nids, $tracked_habitat_nids));
  $existing_tids = $config->get('tracked_tids');
  $config->set('tracked_tids', array_merge($existing_tids, $tracked_habitat_tids));
  $existing_types = $config->get('tracked_content_types');
  $config->set('tracked_content_types', array_merge($existing_types, array('habitat')));
  $existing_vocabs = $config->get('tracked_vocabularies');
  $config->set('tracked_vocabularies', array_merge($existing_vocabs, array('habitat_type')));
  $existing_fields = $config->get('tracked_fields');
  $config->set('tracked_fields', array_merge($existing_fields, array('field_animal_habitat', 'field_habitat_type')));
  $config->set('extras_added.habitats', TRUE);
  $config->save();

  backdrop_set_message(t('Created Habitat content type with @count habitat nodes and hierarchical taxonomy. Animal nodes updated with habitat references.', array(
    '@count' => count($tracked_habitat_nids),
  )));
}

/**
 * Returns sample comment data for animal nodes.
 *
 * Each entry has a 'node_match' key (lowercase, underscored animal title) and
 * 'threads' containing top-level comments with optional nested replies.
 * The 'age_offset' is seconds before REQUEST_TIME for realistic timestamps.
 *
 * @return array
 *   Structured comment data.
 */
function sample_animal_content_comment_data() {
  return array(
    'lion' => array(
      'node_match' => 'lion',
      'threads' => array(
        array(
          'name' => 'Safari Sam',
          'body' => 'I saw a pride of lions on my trip to Kenya last year. The male had the most magnificent mane I have ever seen!',
          'age_offset' => 86400 * 30,
          'replies' => array(
            array(
              'name' => 'Wildlife Watcher',
              'body' => 'Which national park were you visiting? I am planning a trip next year.',
              'age_offset' => 86400 * 28,
              'replies' => array(
                array(
                  'name' => 'Safari Sam',
                  'body' => 'It was the Maasai Mara. Go in August or September for the wildebeest migration - the lions follow the herds.',
                  'age_offset' => 86400 * 27,
                ),
              ),
            ),
            array(
              'name' => 'Dr. Jane',
              'body' => 'Male lions actually spend most of their time resting. They can sleep up to 20 hours a day!',
              'age_offset' => 86400 * 25,
            ),
          ),
        ),
      ),
    ),
    'elephant' => array(
      'node_match' => 'elephant',
      'threads' => array(
        array(
          'name' => 'Trunk Enthusiast',
          'body' => 'Elephants are incredibly intelligent. They can recognize themselves in mirrors and mourn their dead.',
          'age_offset' => 86400 * 45,
          'replies' => array(
            array(
              'name' => 'Dr. Jane',
              'body' => 'They also have excellent long-term memory. An elephant never forgets is more than just a saying.',
              'age_offset' => 86400 * 43,
            ),
          ),
        ),
        array(
          'name' => 'Conservation Carl',
          'body' => 'The biggest threat to elephants is habitat loss, even more than poaching these days. We need to protect wildlife corridors.',
          'age_offset' => 86400 * 20,
        ),
      ),
    ),
    'eagle' => array(
      'node_match' => 'bald_eagle',
      'threads' => array(
        array(
          'name' => 'Birdwatcher Betty',
          'body' => 'Spotted a nesting pair near the river last weekend. The wingspan on these birds is just breathtaking.',
          'age_offset' => 86400 * 14,
          'replies' => array(
            array(
              'name' => 'Ranger Rick',
              'body' => 'Please remember to keep your distance from nesting sites. Federal law protects bald eagles and their nests.',
              'age_offset' => 86400 * 13,
            ),
          ),
        ),
      ),
    ),
    'panda' => array(
      'node_match' => 'giant_panda',
      'threads' => array(
        array(
          'name' => 'Bamboo Bob',
          'body' => 'Did you know pandas spend about 12 hours a day eating? They need to consume around 38 kg of bamboo daily.',
          'age_offset' => 86400 * 60,
          'replies' => array(
            array(
              'name' => 'Zoo Volunteer',
              'body' => 'At our zoo the pandas also get sweet potatoes and special biscuits as supplements. They love the biscuits!',
              'age_offset' => 86400 * 58,
              'replies' => array(
                array(
                  'name' => 'Bamboo Bob',
                  'body' => 'That makes sense. Bamboo is not very nutritious so they need all the help they can get.',
                  'age_offset' => 86400 * 57,
                ),
              ),
            ),
          ),
        ),
      ),
    ),
    '_generic' => array(
      'node_match' => NULL,
      'threads' => array(
        array(
          'name' => 'Animal Lover',
          'body' => 'What a fascinating creature! I would love to see one in the wild someday.',
          'age_offset' => 86400 * 10,
          'replies' => array(
            array(
              'name' => 'Nature Guide',
              'body' => 'Eco-tourism is a great way to see animals in their natural habitat while supporting conservation efforts.',
              'age_offset' => 86400 * 9,
            ),
          ),
        ),
      ),
    ),
  );
}

/**
 * Returns hierarchical habitat type terms.
 *
 * @return array
 *   Keyed array of term data with optional 'parent' references.
 */
function sample_animal_content_habitat_terms() {
  return array(
    'terrestrial' => array(
      'name' => 'Terrestrial',
      'parent' => NULL,
    ),
    'grassland' => array(
      'name' => 'Grassland',
      'parent' => 'terrestrial',
    ),
    'forest' => array(
      'name' => 'Forest',
      'parent' => 'terrestrial',
    ),
    'desert' => array(
      'name' => 'Desert',
      'parent' => 'terrestrial',
    ),
    'tundra' => array(
      'name' => 'Tundra',
      'parent' => 'terrestrial',
    ),
    'aquatic' => array(
      'name' => 'Aquatic',
      'parent' => NULL,
    ),
    'freshwater' => array(
      'name' => 'Freshwater',
      'parent' => 'aquatic',
    ),
    'wetland' => array(
      'name' => 'Wetland',
      'parent' => 'aquatic',
    ),
  );
}

/**
 * Returns sample habitat node data.
 *
 * @return array
 *   Array of habitat definitions with node reference targets.
 */
function sample_animal_content_habitat_data() {
  return array(
    array(
      'title' => 'African Savanna',
      'body' => 'The African savanna is a vast tropical grassland stretching across much of sub-Saharan Africa. Characterized by scattered trees, seasonal rainfall, and warm temperatures year-round, it supports some of the most iconic wildlife on Earth.',
      'type_key' => 'grassland',
      'animals' => array('Lion', 'Elephant', 'Giraffe', 'Gray Wolf'),
    ),
    array(
      'title' => 'Bamboo Forest',
      'body' => 'Bamboo forests are dense stands of bamboo found primarily in the mountainous regions of central China. These cool, misty forests provide essential habitat for some of the world\'s most specialized herbivores.',
      'type_key' => 'forest',
      'animals' => array('Giant Panda'),
    ),
    array(
      'title' => 'South American Highlands',
      'body' => 'The highlands of South America, particularly the Andes and Altiplano, are home to hardy animals adapted to high altitude, thin air, and dramatic temperature swings between day and night.',
      'type_key' => 'grassland',
      'animals' => array('Llama'),
    ),
    array(
      'title' => 'North American Wetlands',
      'body' => 'The rivers, lakes, and coastal wetlands of North America support an incredible diversity of wildlife. Old-growth trees along waterways provide nesting sites for large birds of prey.',
      'type_key' => 'wetland',
      'animals' => array('Bald Eagle'),
    ),
    array(
      'title' => 'Tropical River Systems',
      'body' => 'Tropical river systems and their associated estuaries, mangroves, and floodplains are among the most productive ecosystems on the planet. Large predators patrol these warm, murky waters.',
      'type_key' => 'freshwater',
      'animals' => array('Saltwater Crocodile'),
    ),
    array(
      'title' => 'Temperate Woodlands',
      'body' => 'Temperate woodlands and mixed forests span much of the Northern Hemisphere. These adaptable ecosystems with their mix of deciduous and coniferous trees support highly versatile predators.',
      'type_key' => 'forest',
      'animals' => array('Fox'),
    ),
  );
}
