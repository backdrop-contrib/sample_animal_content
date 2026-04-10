Sample Animal Content
===================

DESCRIPTION
-----------

A module that creates an 'Animal' content type and sample nodes for a Backdrop
CMS site. Useful for theming, development, and testing content import/export
workflows. Animal descriptions sourced from Wikipedia.

On install, the module creates 8 animal nodes with images, body text, taxonomy
terms, and link fields. A configuration page provides options to add more
content for testing richer scenarios.

INSTALLED BY DEFAULT
--------------------

Content type: Animal
- Title
- Body
- Class (taxonomy term reference)
- Image (images included for all nodes)
- Link (Wikipedia URL)

Vocabulary: Class
- Aves
- Mammalia
- Reptilia

Nodes (8):
- Bald Eagle
- Elephant
- Fox
- Giant Panda
- Giraffe
- Lion
- Llama
- Saltwater Crocodile

CONFIGURATION PAGE
------------------

Visit **Administration > Configuration > Content > Sample Animal Content**
(`admin/config/content/sample-animal-content`) to add optional extras:

### Additional Animals

Adds 10 more animal nodes with three new taxonomy classes:

- Sea Turtle (Reptilia)
- Emperor Penguin (Aves)
- Bottlenose Dolphin (Mammalia)
- Poison Dart Frog (Amphibia)
- Great White Shark (Chondrichthyes)
- Great Horned Owl (Aves)
- Gray Wolf (Mammalia)
- Panther Chameleon (Reptilia)
- Common Octopus (Cephalopoda)
- Brown Bear (Mammalia)

### Sample Comments

Adds threaded comments (up to 3 levels deep) on animal nodes. Enables the
Comment module and opens comments on the Animal content type if not already
enabled. Includes specific comments for several animals and generic comments
for the rest.

### Sample Habitats (Entity References)

Creates a Habitat content type with a hierarchical taxonomy and sample nodes,
then adds a Habitat entity reference field to the Animal content type. Enables
the Entity Reference module if not already enabled.

Habitat content type fields:
- Description (body)
- Habitat Type (hierarchical taxonomy term reference)

Vocabulary: Habitat Type
- Terrestrial: Grassland, Forest, Desert, Tundra
- Aquatic: Freshwater, Wetland

Habitat nodes (6):
- African Savanna (Lion, Elephant, Giraffe, Gray Wolf)
- Bamboo Forest (Giant Panda)
- South American Highlands (Llama)
- North American Wetlands (Bald Eagle)
- Tropical River Systems (Saltwater Crocodile)
- Temperate Woodlands (Fox)

TRACKING
--------

The module tracks all content it creates (node IDs, term IDs, content types,
vocabularies, and fields) in its configuration. This tracking is the foundation
for a planned safe-removal feature that will only delete content that has not
been modified by a person.

LICENSE
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory
for complete text.

CURRENT MAINTAINERS
--------------------

- Tim Erickson (https://github.com/stpaultim/)

CREDITS
-------

Development supported by Simplo (by Triplo) - https://simplo.site
