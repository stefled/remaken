### Remaken evolutions
- Update remaken version to 1.7.0
- Update builddefs/qmake to 4.6.0
- Add support for conan index: conan-center packages url are created without appending the '@user/channel' suffix, for packages from other remotes the '@user/channel' suffix is still added.
- Improve init command: now init command can take a --tag option to initialize qmake rules from the tag provided. The --tag value can also be "latest" in which case the latest qmake rules are installed from the repository.


